import json
import xml.etree.ElementTree as et

root = et.parse('ci-run-info.xml').getroot()
cinf = dict()
for child in list(root):
    cinf[child.tag] = [child.text, child.attrib]

oses = [
    "ubuntu-20-04",
    "ubuntu-18-04",
    "centos-7",
    'macos-10-15',
    'macos-11-01'
]

osvers = root.attrib['name']
nodes = [*cinf]

def startswith_any(s, prefix):
    for pre in prefix:
        if s.startswith(pre):
            return True
    return False

def update_value(key, value):
    cinf[key][0] = value

def valueof(key):
    return cinf[key][0]

def linesof(key):
    return valueof(key).splitlines()

def attrsof(key):
    return cinf[key][1]

def attr(ekey, akey):
    return attrsof(ekey)[akey]

def pkg_obj(pkg, vers, repo):
    return { 'package': pkg, 'version': vers, 'repository': repo }

def get_changes(a, b):
    pre, post = (set(a), set(b))
    return pre, post, (post - pre)

def pkg_changes(a, b):
    pre, post, diff = get_changes(a, b)
    if osvers == 'centos-7':
        return [[pkg_obj(*pkgln.split()) for pkgln in lns] for lns in [pre, post, diff]]
    raise NotImplementedError(f'not implemented for {osvers}')

def jsonify_pkg_deps():
    cinf['packages'] = [None]
    if osvers == 'centos-7':
        pkgs = set(linesof('packages-bdm')[1:]).union(set(linesof('packages-bdm-all')[1:]))
        cinf['packages'][0] = [pkg_obj(*pkgln.split()) for pkgln in pkgs]
    else:
        raise NotImplementedError(f'not implemented for {osvers}')


bdmvers = valueof('bdm-version')
timestamp = valueof('timestamp')

update_value('bdm-config', '\n'.join(valueof('bdm-config').split()))
jsonify_pkg_deps()

keys_to_drop = { 'packages-pre-bdm', 'packages-bdm', 'packages-bdm-all',
'timestamp', 'bdm-version', 'environment-pre-bdm',
'environment-bdm', 'dependency-graph', 'modules-pre-bdm', 'modules-bdm' }

dump = json.dumps({
    "release": {
        "os": osvers,
        "version": bdmvers,
        "timestamp": timestamp,
        "info": { k: v[0] for (k,v) in cinf.items() if not k in keys_to_drop }
    }
})

with open(f"{osvers}.{valueof('bdm-version')}.json", "wt") as json_out:
    json_out.write(dump)
