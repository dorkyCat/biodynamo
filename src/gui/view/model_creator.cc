// Author: Lukasz Stempniewicz 25/05/19

// -----------------------------------------------------------------------------
//
// Copyright (C) The BioDynaMo Project.
// All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
//
// See the LICENSE file distributed with this work for details.
// See the NOTICE file distributed with this work for additional information
// regarding copyright ownership.
//
// -----------------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// This File contains the implementation of the ModelCreator-class      //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#include <Riostream.h>
#include <stdlib.h>
#include <time.h>
#include <stdexcept>
#include <string>

#include <KeySymbols.h>
#include <TEnv.h>
#include <TROOT.h>
#include <TRint.h>
#include <TStyle.h>
#include <TVirtualX.h>

#include "gui/view/about.h"
#include "gui/view/button_model.h"
#include "gui/view/button_project.h"
#include "gui/constants.h"
#include "gui/controller/project.h"
#include "gui/view/help_text.h"
#include "gui/view/log.h"
#include "gui/view/model_creator.h"
#include "gui/view/model_frame.h"
#include "gui/view/new_dialog.h"
#include "gui/view/title.h"
#include "gui/view/tree_manager.h"

#include "biodynamo.h"

namespace bdm {

/// testing
inline Cell* GetCell() {
  const std::array<double, 3> position = {1, 2, 3};
  Cell *c1 = new Cell(position);
  return c1;
}

/// testing
inline Cell* GetGrowthModule() {
  const std::array<double, 3> position = {1, 2, 3};
  Cell *c1 = new Cell(position);
  return c1;
}

}

namespace gui {

const char *icon_names[] = {"new_project.xpm",
                            "",
                            "open.xpm",
                            "save.xpm",
                            "settings.xpm",
                            "",
                            "build.png",
                            "run.png",
                            "generate_code.xpm",
                            "",
                            "browser.xpm",
                            "",
                            "user_guide.xpm",
                            "dev_guide.xpm",
                            "license.xpm",
                            "about.xpm",
                            "",
                            "quit.xpm",
                            0};

ToolBarData_t tb_data[] = {
    {"", "New Project", kFALSE, M_FILE_NEWPROJECT, NULL},
    {"", 0, 0, -1, NULL},
    {"", "Open Root project file", kFALSE, M_FILE_OPENPROJECT, NULL},
    {"", "Save project in Root file", kFALSE, M_FILE_SAVE, NULL},
    {"", "Project Preferences", kFALSE, M_FILE_PREFERENCES, NULL},
    {"", 0, 0, -1, NULL},
    {"", "Build current model", kFALSE, M_SIMULATION_BUILD, NULL},
    {"", "Run current model", kFALSE, M_SIMULATION_RUN, NULL},
    {"", "Generate BioDynaMo code", kFALSE, M_SIMULATION_GENERATE, NULL},
    {"", 0, 0, -1, NULL},
    {"", "Start Root browser", kFALSE, M_TOOLS_STARTBROWSER, NULL},
    {"", 0, 0, -1, NULL},
    {"", "Developer Guide", kFALSE, M_HELP_USERGUIDE, NULL},
    {"", "User Guide", kFALSE, M_HELP_DEVGUIDE, NULL},
    {"", "Display License", kFALSE, M_HELP_LICENSE, NULL},
    {"", "About Model Creator", kFALSE, M_HELP_ABOUT, NULL},
    {"", 0, 0, -1, NULL},
    {"", "Exit Application", kFALSE, M_FILE_EXIT, NULL},
    {NULL, NULL, 0, 0, NULL}};

const char *filetypes[] = {"ROOT files", "*.root", "All files", "*", 0, 0};

ModelCreator *gModelCreator;

//_________________________________________________
// ModelCreator
//

Int_t ModelCreator::fgDefaultXPosition = 20;
Int_t ModelCreator::fgDefaultYPosition = 20;

////////////////////////////////////////////////////////////////////////////////
/// Create (the) Model Display.
///
/// p = pointer to GMainFrame (not owner)
/// w = width of ModelCreator frame
/// h = width of ModelCreator frame

ModelCreator::ModelCreator(const TGWindow *p, UInt_t w, UInt_t h)
    : TGMainFrame(p, w, h) {
  // Project::GetInstance();
  fOk = kFALSE;
  fModified = kFALSE;
  fSettingsModified = kFALSE;
  fIsRunning = kFALSE;
  fInterrupted = kFALSE;
  fIsNewProject = kFALSE;

  fModelCreatorEnv = std::make_unique<TEnv>(".modelcreatorrc");  //  not yet used

  fTreeManager = std::make_unique<TreeManager>();

  fProjectName.clear();
  fProjectPath.clear();

  /// Create menubar and popup menus.
  MakeMenuBarFrame();

  ///---- toolbar
  int spacing = 8;
  fToolBar = std::make_unique<TGToolBar>(this, 60, 20, kHorizontalFrame | kRaisedFrame);
  for (int i = 0; icon_names[i]; i++) {
    TString iconname(gProgPath);
    #ifdef R__WIN32
      iconname += "\\icons\\";
    #else
      iconname += "/icons/";
    #endif
    iconname += icon_names[i];
    tb_data[i].fPixmap = iconname.Data();
    if (strlen(icon_names[i]) == 0) {
      fToolBar->AddFrame(new TGVertical3DLine(fToolBar.get()),
                         new TGLayoutHints(kLHintsExpandY, 4, 4));
      continue;
    }
    const TGPicture *pic = fClient->GetPicture(tb_data[i].fPixmap);
    TGPictureButton *pb = new TGPictureButton(fToolBar.get(), pic, tb_data[i].fId);
    pb->SetToolTipText(tb_data[i].fTipText);
    tb_data[i].fButton = pb;

    fToolBar->AddButton(this, pb, spacing);
    spacing = 0;
  }
  AddFrame(fToolBar.get(),
           new TGLayoutHints(kLHintsTop | kLHintsExpandX, 0, 0, 0, 0));
  fToolBar->GetButton(M_FILE_SAVE)->SetState(kButtonDisabled);

  /// Layout hints
  fL1 = std::make_unique<TGLayoutHints>(kLHintsCenterX | kLHintsExpandX, 0, 0, 0, 0);
  fL2 = std::make_unique<TGLayoutHints>(
      kLHintsBottom | kLHintsLeft | kLHintsExpandX | kLHintsExpandY, 2, 2, 2,
      2);
  fL3 = std::make_unique<TGLayoutHints>(kLHintsBottom | kLHintsExpandX, 0, 0, 0, 0);

  // CREATE TITLE FRAME
  fTitleFrame = std::make_unique<TitleFrame>(this, "BioDyanMo", "Model Creator", 100, 100);
  AddFrame(fTitleFrame.get(), fL1.get());

  // CREATE MAIN FRAME
  fMainFrame = std::make_unique<TGCompositeFrame>(this, 100, 100, kHorizontalFrame | kRaisedFrame);

  TGVerticalFrame *fV1 =
      new TGVerticalFrame(fMainFrame.get(), 150, 10, kSunkenFrame | kFixedWidth);

  TGLayoutHints *lo;

  lo = new TGLayoutHints(kLHintsLeft | kLHintsExpandY, 2, 0, 2, 2);
  fMainFrame->AddFrame(fV1, lo);

  TGVSplitter *splitter = new TGVSplitter(fMainFrame.get(), 5);
  splitter->SetFrame(fV1, kTRUE);
  lo = new TGLayoutHints(kLHintsLeft | kLHintsExpandY, 0, 0, 0, 0);
  fMainFrame->AddFrame(splitter, lo);

  lo = new TGLayoutHints(kLHintsRight | kLHintsExpandX | kLHintsExpandY, 0, 2,
                         2, 2);

  // Create Selection frame (i.e. with buttons and selection widgets)
  fSelectionFrame = std::make_unique<TGCompositeFrame>(fV1, 100, 100, kVerticalFrame);

  fButtonModelFrame = std::make_unique<ButtonModelFrame>(fSelectionFrame.get(), this, M_MODEL_NEW,
                                           M_MODEL_SIMULATE, M_INTERRUPT_SIMUL);

  // create project button frame
  fButtonProjectFrame = std::make_unique<ButtonProjectFrame>(
      fSelectionFrame.get(), this, M_FILE_NEWPROJECT, M_FILE_OPENPROJECT);
  lo = new TGLayoutHints(kLHintsTop | kLHintsCenterX | kLHintsExpandX, 2, 5, 1,
                         2);

  fSelectionFrame->AddFrame(fButtonProjectFrame.get(), lo);
  fSelectionFrame->AddFrame(fButtonModelFrame.get(), lo);

  fTreeView =
      std::make_unique<TGCanvas>(fSelectionFrame.get(), 150, 10, kSunkenFrame | kDoubleBorder);
  fProjectListTree =
      std::make_unique<TGListTree>(fTreeView->GetViewPort(), 10, 10, kHorizontalFrame);
  fProjectListTree->SetCanvas(fTreeView.get());
  fProjectListTree->Associate(this);
  fTreeView->SetContainer(fProjectListTree.get());
  fSelectionFrame->AddFrame(fTreeView.get(), fL2.get());

  lo = new TGLayoutHints(kLHintsExpandX | kLHintsExpandY);
  fV1->AddFrame(fSelectionFrame.get(), lo);

  // Create Display frame
  fModelFrame = std::make_unique<ModelFrame>(fMainFrame.get(), this);
  fModelFrame->EnableButtons(M_NONE_ACTIVE);
  fMainFrame->AddFrame(fModelFrame.get(), lo);

  // Create Display Canvas Tab (where the actual models are displayed)
  // TGCompositeFrame *tFrame = fDisplayFrame->AddTab("Untitled Model");

  // TODO: add more to frames/tabs/etc

  AddFrame(fMainFrame.get(), lo);

  // Create status bar
  Int_t parts[] = {45, 45, 10};
  fStatusBar = std::make_unique<TGStatusBar>(this, 50, 10, kHorizontalFrame);
  fStatusBar->SetParts(parts, 3);
  AddFrame(fStatusBar.get(), fL3.get());
  Log::SetStatusBar(fStatusBar.get());
  fStatusBar->SetText("Please create or load a project", 0);

  // Finish ModelCreator for display...
  SetWindowName("BioDynaMo Model Creator");
  SetIconName("BioDynaMo Model Creator");

  MapSubwindows();
  Resize();  // this is used here to init layout algorithm
  MapWindow();

  ChangeSelectionFrame(kFALSE);

  // fEvent = new MyEvent();
  // fEvent->GetDetector()->Init();
  // fEvent->Init(0, fFirstParticle, fE0, fB);
  // Initialize();
  // gROOT->GetListOfBrowsables()->Add(fEvent,"RootShower Event");
  gSystem->Load("libTreeViewer");
  AddInput(kKeyPressMask | kKeyReleaseMask);
  gVirtualX->SetInputFocus(GetId());
  gModelCreator = this;
}

////////////////////////////////////////////////////////////////////////////////
/// Create menubar and popup menus.

void ModelCreator::MakeMenuBarFrame() {
  /// layout hint items
  fMenuBarLayout = std::make_unique<TGLayoutHints>(kLHintsTop | kLHintsLeft | kLHintsExpandX, 0, 0, 0, 0);
  fMenuBarItemLayout = std::make_unique<TGLayoutHints>(kLHintsTop | kLHintsLeft, 0, 4, 0, 0);
  fMenuBarHelpLayout = std::make_unique<TGLayoutHints>(kLHintsTop | kLHintsRight);
  fMenuBar = std::make_unique<TGMenuBar>(this, 1, 1, kHorizontalFrame | kRaisedFrame);

  /// Menu - File
  fMenuFile = std::make_unique<TGPopupMenu>(gClient->GetRoot());
  fMenuFile->AddEntry("&New Project", M_FILE_NEWPROJECT);
  fMenuFile->AddEntry("&Open Project...", M_FILE_OPENPROJECT);
  fMenuFile->AddEntry("&Save", M_FILE_SAVE);
  fMenuFile->AddEntry("S&ave as...", M_FILE_SAVEAS);
  fMenuFile->AddSeparator();
  fMenuFile->AddEntry("Import", M_FILE_IMPORT);
  fMenuFile->AddEntry("Export", M_FILE_EXPORT);
  fMenuFile->AddSeparator();
  fMenuFile->AddEntry("Preferences", M_FILE_PREFERENCES);
  fMenuFile->AddSeparator();
  fMenuFile->AddEntry("E&xit", M_FILE_EXIT);

  fMenuFile->DisableEntry(M_FILE_SAVE);
  fMenuFile->DisableEntry(M_FILE_SAVEAS);

  /// Menu - Simulation
  fMenuSimulation = std::make_unique<TGPopupMenu>(gClient->GetRoot());
  fMenuSimulation->AddEntry("Generate BioDynaMo code", M_SIMULATION_GENERATE);
  fMenuSimulation->AddSeparator();
  fMenuSimulation->AddEntry("Build", M_SIMULATION_BUILD);
  fMenuSimulation->AddEntry("Run", M_SIMULATION_RUN);
  fMenuSimulation->AddSeparator();
  fMenuSimulation->AddEntry("Open Paraview", M_SIMULATION_OPENPARAVIEW);

  fMenuSimulation->DisableEntry(M_SIMULATION_GENERATE);
  fMenuSimulation->DisableEntry(M_SIMULATION_BUILD);
  fMenuSimulation->DisableEntry(M_SIMULATION_RUN);
  fMenuSimulation->DisableEntry(M_SIMULATION_OPENPARAVIEW);

  /// Menu - Tools
  fMenuTools = std::make_unique<TGPopupMenu>(gClient->GetRoot());
  fMenuTools->AddLabel("Tools...");
  fMenuTools->AddSeparator();
  fMenuTools->AddEntry("Start &Browser\tCtrl+B", M_TOOLS_STARTBROWSER);

  /// Menu - View
  fMenuView = std::make_unique<TGPopupMenu>(gClient->GetRoot());
  fMenuView->AddEntry("&Toolbar", M_VIEW_TOOLBAR);
  fMenuView->CheckEntry(M_VIEW_TOOLBAR);

  /// Menu - Samples
  fMenuSamples = std::make_unique<TGPopupMenu>(gClient->GetRoot());
  fMenuSamples->AddLabel("Try out demo projects...");
  fMenuSamples->AddSeparator();
  fMenuSamples->AddEntry("Cell Division", M_SAMPLES_CELLDIVISION);
  fMenuSamples->AddEntry("Diffusion", M_SAMPLES_DIFFUSION);
  fMenuSamples->AddEntry("Gene Regulation", M_SAMPLES_GENEREGULATION);
  fMenuSamples->AddEntry("Some Clustering", M_SAMPLES_SOMACLUSTERING);
  fMenuSamples->AddEntry("Tumor Concept", M_SAMPLES_TUMORCONCEPT);

  fMenuSamples->DisableEntry(M_SAMPLES_CELLDIVISION);
  fMenuSamples->DisableEntry(M_SAMPLES_DIFFUSION);
  fMenuSamples->DisableEntry(M_SAMPLES_GENEREGULATION);
  fMenuSamples->DisableEntry(M_SAMPLES_SOMACLUSTERING);
  fMenuSamples->DisableEntry(M_SAMPLES_TUMORCONCEPT);

  /// Menu - Help
  fMenuHelp = std::make_unique<TGPopupMenu>(gClient->GetRoot());
  fMenuHelp->AddEntry("User Guide", M_HELP_USERGUIDE);
  fMenuHelp->AddEntry("Dev Guide", M_HELP_DEVGUIDE);
  fMenuHelp->AddSeparator();
  fMenuHelp->AddEntry("About", M_HELP_ABOUT);

  fMenuHelp->DisableEntry(M_HELP_USERGUIDE);
  fMenuHelp->DisableEntry(M_HELP_DEVGUIDE);

  /// Associate signals
  fMenuFile->Associate(this);
  fMenuSamples->Associate(this);
  fMenuTools->Associate(this);
  fMenuView->Associate(this);
  fMenuSimulation->Associate(this);
  fMenuHelp->Associate(this);
  fMenuFile->Associate(this);

  fMenuBar->AddPopup("&File", fMenuFile.get(), fMenuBarItemLayout.get());
  fMenuBar->AddPopup("S&imulation", fMenuSimulation.get(), fMenuBarItemLayout.get());
  fMenuBar->AddPopup("&Tools", fMenuTools.get(), fMenuBarItemLayout.get());
  fMenuBar->AddPopup("&View", fMenuView.get(), fMenuBarItemLayout.get());
  fMenuBar->AddPopup("&Samples", fMenuSamples.get(), fMenuBarItemLayout.get());
  fMenuBar->AddPopup("&Help", fMenuHelp.get(), fMenuBarHelpLayout.get());

  AddFrame(fMenuBar.get(), fMenuBarLayout.get());
}


////////////////////////////////////////////////////////////////////////////////
/// Show or hide toolbar.

void ModelCreator::ShowToolBar(Bool_t show) {
  if (show) {
    ShowFrame(fToolBar.get());
    fMenuView->CheckEntry(M_VIEW_TOOLBAR);
  } else {
    HideFrame(fToolBar.get());
    fMenuView->UnCheckEntry(M_VIEW_TOOLBAR);
  }
}

void ModelCreator::EnableSaving(Bool_t enable) {
  if(enable) {
    fMenuFile->EnableEntry(M_FILE_SAVE);
    fMenuFile->EnableEntry(M_FILE_SAVEAS);
    fToolBar->GetButton(M_FILE_SAVE)->SetState(kButtonUp);
  } else {
    fMenuFile->DisableEntry(M_FILE_SAVE);
    fMenuFile->DisableEntry(M_FILE_SAVEAS);
    fToolBar->GetButton(M_FILE_SAVE)->SetState(kButtonDisabled);
  }
  
}

////////////////////////////////////////////////////////////////////////////////
/// Set the default position on the screen of new Model Creator instances.

void ModelCreator::SetDefaultPosition(Int_t x, Int_t y) {
  fgDefaultXPosition = x;
  fgDefaultYPosition = y;
}

////////////////////////////////////////////////////////////////////////////////
/// Creates a new Project within model creator,
/// both fProjectName and fProjectPath should be set.
void ModelCreator::CreateNewProject() {
  if (fProjectName.empty() || fProjectPath.empty()) {
    Log::Error("Project name or path is empty! Cannot create Project!");
    return;
  }
  ClearProject();
  Project::GetInstance().NewProject(fProjectPath.c_str(), fProjectName.c_str());
  /// Previous tree manager will be destroyed
  fTreeManager->CreateProjectTree(fProjectListTree.get(), fProjectName);
  Initialize();
  ChangeSelectionFrame();
}

void ModelCreator::LoadProject(std::string fileName) {
  const char* projectName = Project::GetInstance().LoadProject(fileName.c_str());
  const char* projectLocation = fileName.c_str();
  NewProjectSet(projectName, projectLocation);
  fTreeManager->CreateProjectTree(fProjectListTree.get(), fProjectName);
  std::vector<Model>* models = Project::GetInstance().GetAllModels();
  Size_t modelCount = models->size();
  Log::Info("Number of models:", modelCount);
  for(Int_t i = 0; i < modelCount; i++) {
    Model curModel = models->at(i);
    fTreeManager->CreateModelTree(curModel);
  }
}

Bool_t ModelCreator::AskForProject(Bool_t loading) {
  Log::Warning("Can only have 1 project open!");
  Int_t retval;
  std::string msg("Press OK to save the current project");
  if(loading) {
    msg.append(", then load an existing one.");
  } else {
    msg.append(", then create a new one.");
  }
  new TGMsgBox(gClient->GetRoot(), this,
              "Warning", msg.c_str(),
              kMBIconExclamation, kMBOk | kMBCancel, &retval);
  if(retval == kMBOk) {
    return kTRUE;
  }
  return kFALSE;
}

void ModelCreator::ClearProject() {
  if(Project::GetInstance().IsLoaded()) {
    Project::GetInstance().SaveProject();
    Project::GetInstance().CloseProject();
  }
  fTreeManager = std::make_unique<TreeManager>();
  fProjectListTree->Cleanup();
  ChangeSelectionFrame(kFALSE);
}

void ModelCreator::CreateNewModel() {
  if(Project::GetInstance().CreateModel(fModelName.c_str())) {
    Log::Info("Creating Model on tree:", fModelName);
    fTreeManager->CreateModelTree(fModelName);
    Initialize();
    std::string tmp = fModelName + " Overview";
    fModelFrame->ShowModelElement(fModelName.c_str(), tmp.c_str());
    fClient->NeedRedraw(fModelFrame.get());
  }
}

////////////////////////////////////////////////////////////////////////////////
/// Initialize ModelCreator display.

void ModelCreator::Initialize() {
  Interrupt(kFALSE);
  fProjectListTree->ClearViewPort();
  fClient->NeedRedraw(fProjectListTree.get());

  fStatusBar->SetText("", 1);
}

void ModelCreator::ChangeSelectionFrame(Bool_t createdProject) {
  fSelectionFrame->MapSubwindows();
  if (createdProject) {
    fSelectionFrame->ShowFrame(fButtonModelFrame.get());
    fSelectionFrame->HideFrame(fButtonProjectFrame.get());
  } else {
    fSelectionFrame->HideFrame(fButtonModelFrame.get());
    fSelectionFrame->ShowFrame(fButtonProjectFrame.get());
  }
  fSelectionFrame->MapWindow();
}

void ModelCreator::CreateNewElement(int type) {
  std::string elemName = fTreeManager->CreateTopLevelElement(type);
  Project::GetInstance().CreateModelElement(fModelName.c_str(), "", elemName.c_str(), type);
  fProjectListTree->ClearViewPort();
  fClient->NeedRedraw(fProjectListTree.get());
}

void ExtractMethod(TObject* obj) {
  std::string clName(obj->ClassName());
  if(clName.compare("TMethod") == 0) {
    TMethod* method = (TMethod*)obj;
    std::string methodName(method->GetName());
    if(methodName.find("Set") != std::string::npos) {
      std::string methodSignature(method->GetSignature());
      std::cout << "Setter found:" << methodName << methodSignature << '\n';
    }
      
  }

}

void PrintList(auto&& t, Bool_t useIterator=kFALSE) {

  std::cout << "ClassName:" << t->ClassName() << '\n';
  if(!useIterator) {
    TObjLink* lnk = t->FirstLink();
    while (lnk) {
      TObject* obj = lnk->GetObject();
      ExtractMethod(obj);
      //obj->Print();
      lnk = lnk->Next();
    }
  } else {
    TIterator* it = t->MakeIterator();
    TObject* obj = it->Next();
    while(obj) {
      //obj->Print();
      ExtractMethod(obj);
      obj = it->Next();
    }
  }

}

void ViewMembers() {

  //TBrowser *b = ;
  std::cout << "Testing getting cell members\n";
  bdm::Cell *ptr = bdm::GetCell();
  TClass *cl = ptr->IsA();

  std::cout << "\nData Members:\n";
  PrintList(cl->GetListOfDataMembers());

  std::cout << "\nMethods:\n";
  PrintList(cl->GetListOfMethods());

  std::cout << "\nReal Data:\n";
  PrintList(cl->GetListOfRealData());
 
  std::cout << "\nPublic Methods:\n";
  PrintList(cl->GetListOfAllPublicMethods(), kTRUE);

  std::cout << "\nPublic Data Members:\n";
  PrintList(cl->GetListOfAllPublicDataMembers(), kTRUE);


}

////////////////////////////////////////////////////////////////////////////////
/// Handle messages send to the ModelCreator object.

Bool_t ModelCreator::ProcessMessage(Long_t msg, Long_t param1, Long_t param2) {

  switch (GET_MSG(msg)) {
    case kC_COMMAND:
      switch (GET_SUBMSG(msg)) {
        case kCM_BUTTON:
        case kCM_MENU:
          switch (param1) {
            case M_FILE_NEWPROJECT: {
              if(Project::GetInstance().IsLoaded()) {
                if(AskForProject()) {
                  ClearProject();
                  goto NewProject;
                }
              } else {
                NewProject:
                new NewProjectDialog(fClient->GetRoot(), this, 800, 400);
                CreateNewProject();
                EnableSaving();
                fModified = kTRUE;
              }
              break;
            }

            case M_FILE_OPENPROJECT:
            {
              if(Project::GetInstance().IsLoaded()) {
                if(AskForProject(kTRUE)) {
                  ClearProject();
                  goto LoadingProject;
                }
              } else {
                LoadingProject:
                Log::Debug("Clicked open project!");
                TGFileInfo fi;
                fi.fFileTypes = filetypes;
                new TGFileDialog(fClient->GetRoot(), this, kFDOpen, &fi);
                if (!fi.fFilename) return kTRUE;
                LoadProject(fi.fFilename);
              }
            }
              break;

            case M_MODEL_NEW:
              new NewModelDialog(fClient->GetRoot(), this, 800, 400);
              if (fIsNewModel) {
                fModified = kTRUE;
                fIsNewModel = kFALSE;
                CreateNewModel();
              }
              break;

            case M_MODEL_SIMULATE:
              Log::Debug("Clicked simulate!");
              break;

            case M_INTERRUPT_SIMUL:
              Interrupt();
              break;

            case M_FILE_SAVE:
              Log::Debug("Clicked save!");
              Project::GetInstance().SaveProject();
              fModified = kFALSE;
              break;

            case M_FILE_SAVEAS: 
            {
              /// TODO: SaveAs not yet fully functional
              Log::Debug("Clicked save as!");
              TGFileInfo fi;
              fi.fFileTypes = filetypes;
              new TGFileDialog(fClient->GetRoot(), this, kFDOpen, &fi);
              if (!fi.fFilename) return kTRUE;
              Project::GetInstance().SaveAsProject(fi.fFilename);
            }
              break;

            case M_FILE_EXIT:
              CloseWindow();  // this also terminates theApp
              break;

            case M_FILE_PREFERENCES:
              Log::Info("Clicked preferences!");
              break;

            case M_ENTITY_CELL:
              Log::Debug("Clicked cell!");
              CreateNewElement(M_ENTITY_CELL);
              fModified = kTRUE;
              break;

            case M_MODULE_GROWTH:
              Log::Debug("Clicked growth module!");
              CreateNewElement(M_MODULE_GROWTH);
              fModified = kTRUE;
              break;

            case M_MODULE_CHEMOTAXIS:
              Log::Debug("Clicked chemotaxis module!");
              fModified = kTRUE;
              break;

            case M_MODULE_SUBSTANCE:
              Log::Debug("Clicked substance secretion module!");
              fModified = kTRUE;
              break;

            case M_GENERAL_VARIABLE:
              Log::Debug("Clicked general variable!");
              fModified = kTRUE;
              break;

            case M_GENERAL_FUNCTION:
              Log::Debug("Clicked general function!");
              fModified = kTRUE;
              break;

            case M_GENERAL_FORMULA:
              Log::Debug("Clicked general formula!");
              fModified = kTRUE;
              break;

            case M_TOOLS_STARTBROWSER:
            {
              ViewMembers();
              //new TBrowser;
            }
              break;

            case M_VIEW_TOOLBAR:
              if (fMenuView->IsEntryChecked(M_VIEW_TOOLBAR))
                ShowToolBar(kFALSE);
              else
                ShowToolBar();
              break;

            case M_HELP_LICENSE:
              int ax, ay;
              TRootHelpDialog *hd;
              Window_t wdummy;
              Char_t strtmp[250];
              sprintf(strtmp, "Model Creator License");
              hd = new TRootHelpDialog(this, strtmp, 640, 380);
              hd->SetText(gHelpLicense);
              gVirtualX->TranslateCoordinates(
                  GetId(), GetParent()->GetId(), (Int_t)(GetWidth() - 640) >> 1,
                  (Int_t)(GetHeight() - 380) >> 1, ax, ay, wdummy);
              hd->Move(ax, ay);
              hd->Popup();
              fClient->WaitFor(hd);
              break;

            case M_HELP_ABOUT:
              new ModelCreatorAbout(gClient->GetRoot(), this, 400, 200);
              break;

          }       // switch param1
          break;  // M_MENU
      }       // switch submsg
      break;  // case kC_COMMAND

    case kC_LISTTREE:
      switch (GET_SUBMSG(msg)) {
        case kCT_ITEMDBLCLICK:
          if (param1 == kButton1) {
            if (fProjectListTree->GetSelected()) {
              fProjectListTree->ClearViewPort();
              fClient->NeedRedraw(fProjectListTree.get());
            }
          }
          break;

        case kCT_ITEMCLICK:
          HandleTreeInput();
          break;

      }       // switch submsg
      break;  // case kC_LISTTREE
  }           // switch msg

  return kTRUE;
}

////////////////////////////////////////////////////////////////////////////////
/// Process mouse clicks in TGListTree.

void ModelCreator::HandleTreeInput() {
  fProjectListTree->ClearViewPort();
  std::string selectedModelName = fTreeManager->IsModelSelected();
  if (!selectedModelName.empty()) {
    Log::Info("Selected part of ", selectedModelName);
    fModelFrame->EnableButtons(M_ALL_ACTIVE);
    fModelFrame->SwitchModelTab(selectedModelName.c_str());
  } else {
    fModelFrame->EnableButtons(M_NONE_ACTIVE);
  }
  std::string itemName(fProjectListTree->GetSelected()->GetText());
  if (itemName.find("Cell") != std::string::npos ||
      itemName.find("Growth") != std::string::npos) {
    fModelFrame->ShowModelElement(selectedModelName.c_str(),
                                  itemName.c_str());
    fClient->NeedRedraw(fModelFrame.get());
    fModelFrame->Resize(10000, 10000);
    fModelFrame->Resize(10001, 10001);
  }
}

////////////////////////////////////////////////////////////////////////////////
/// Got close message for this Model Creator. The ModelDislay and the
/// application will be terminated.

void ModelCreator::CloseWindow() {
  if(fModified) {
    Int_t retval;
    new TGMsgBox(gClient->GetRoot(), this,
                "Info", "Any unsaved changes will be lost! Press OK to continue.",
                kMBIconExclamation, kMBOk | kMBCancel, &retval);
    if(retval != kMBOk) {
      return;
    }
  }

  std::cout << "Terminating Model Creator" << std::endl;

  //this->DeleteWindow();
  gApplication->Terminate();
}

}  // namespace gui