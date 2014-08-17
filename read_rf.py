import fileinput
import tarfile
import re

class Node:
    """Properties follow those detailed in README.cch"""
    def __init__(self, uid, loc, dns, bb, num_neigh, ext, nuids, euids, name, alias, rn):
        self.uid = int(uid)
        if self.uid < 0:
            raise ValueError
        self.loc = loc
        self.dns = True if dns=='+' else False
        self.bb = True if bb=='bb' else False
        self.num_neigh = int(num_neigh)
        self.ext = int(ext) if ext else 0
        self.nuids = [int(nuid) for nuid in nuids]
        if len(nuids) != self.num_neigh:
            raise ValueError
        self.euids = [int(euid) for euid in euids]
        if len(self.euids) != self.ext:
            raise ValueError
        self.name = name
        self.alias = True if alias=='!' else False
        self.rn = int(rn)
        if len(nuids) != num_neigh:
            print("len(nuids) != num_neigh")

class Ext:
    def __init__(self, euid, address, rn):
        self.euid = euid
        self.address = address
        self.rn = rn

class AS:
    def __init__(self):
        self.nodes = []
        self.exts = []
        self.names = {}
        self.links = set()
        self.link_count = 0

    def add_node(self, node):
        self.nodes.append(node)
        self.link_count += node.num_neigh + node.ext # counts both ways
        uid = node.uid
        # avoid adding reverse of existing tuple/link
        for nuid in node.nuids:
            if (nuid,uid) not in self.links:
                self.links.add((uid,nuid))

fn = "rocketfuel_maps_cch.tar.gz"
archive = tarfile.open(fn)
asn = '4755'
file0 = archive.extractfile(asn+'.cch')

# lines to consider:
# 1 @Adelaide,+Australia  	(1) -> <1787> 	=firstc1-new.link.telstra.net.136.130.139.in-addr.! r1
# 15 @Melbourne,+Australia  	(1) &6 -> <3471> {-1184} {-952} {-949} {-931} {-859} {-64} 	=203.24.109.1 r2
# 99 @T  bb	(4) &1 -> <1036> <1500> <2483> <4298> {-219} 	=203.35.151.1 r1
# 131 @Port+Pirie,+Australia  	(1) -> <2832> 	=203.58.167.1 r2
# 398 @Perth,+Australia + 	(2) -> <3856> <3888> 	=139.130.73.113 r0

uid_re = re.compile(r'(?P<uid>\d+) '
                  r'@(?P<loc>\S+) '
                  r'(?P<dns>\+)? '
                  r'(?P<bb>bb)?\t'
                  r'\((?P<num_neigh>\d+)\) '
                  r'(&(?P<ext>\d+) )?-> '
                  r'(<(?P<nuid>[\d<> ]+)> )?'
                  r'(\{(?P<euid>[-\d{} ]+)\} )?\t'
                  r'=(?P<name_alias>\S+) '
                  r'r(?P<rn>\d+)'
                  )

euid_re = re.compile(r'-(?P<euid>\d+) '
                     r'=(?P<address>\S+) '
                     r'r(?P<rn>\d+)'
                     )

as1221 = AS()
exts = []
for line in file0:
    m = uid_re.match(line.decode('UTF-8'))
    if m != None:
        if m.group('euid'):
            ext_uids = [int(x) for x in m.group('euid').split('} {')]
        else:
            ext_uids = []
        if m.group('name_alias')[-1] == '!':
            name = m.group('name_alias')[:-1]
            alias = '!'
        else:
            name = m.group('name_alias')
            alias = None
        as1221.add_node(
            Node(int(m.group('uid')), m.group('loc'), m.group('dns'),
                 m.group('bb'), int(m.group('num_neigh')), m.group('ext'),
                 [int(x) for x in m.group('nuid').split('> <')], ext_uids,
                 name, alias, int(m.group('rn'))
                 )
            )
        continue
    m = euid_re.match(line.decode('UTF-8'))
    if m != None:
        exts.append(
            Ext(int(m.group('euid')), m.group('address'), int(m.group('rn')))
            )
        continue
    print(line) # unrecognised line
print(len(as1221.nodes))
print(len(exts))

def write_to_ned():
    f = open('as'+asn+'.ned', 'w')
    f.write('network AS'+asn+'\n{\n')
    # python nodes linked by uid, but ned uses list index. need to map.
    uid_to_index = {}
    bb_index = 0
    access_index = 0
    for node in as1221.nodes:
        if node.bb:
            uid_to_index[node.uid] = ('bb', bb_index)
            bb_index += 1
        else:
            uid_to_index[node.uid] = ('access', access_index)
            access_index += 1
#    for i in range(len(as1221.nodes)):
#        uid_to_index[as1221.nodes[i].uid] = i
    f.write(' '*4+'submodules:\n')
    f.write(' '*8+'backbone['+str(bb_index)+']: Core;\n')
    f.write(' '*8+'access['+str(access_index)+']: PoP;\n')
    f.write(' '*4+'connections:\n')
    for (n1,n2) in as1221.links:
        if uid_to_index[n1][0] == 'bb':
            left = 'backbone['+str(uid_to_index[n1][1])
        else: # 'access'
            left = 'access['+str(uid_to_index[n1][1])
        if uid_to_index[n2][0] == 'bb':
            right = 'backbone['+str(uid_to_index[n2][1])
        else: # 'access'
            right = 'access['+str(uid_to_index[n2][1])
        f.write(' '*8+left+'].out++ --> OC12 --> '+right+'].in++;\n')
        f.write(' '*8+left+'].in++ <-- OC12 <-- '+right+'].out++;\n')

#        f.write(' '*8+'internal['+str(uid_to_index[n1])+'].out++ --> OC12 --> internal['+str(uid_to_index[n2])+'].in++;\n')
#        f.write(' '*8+'internal['+str(uid_to_index[n1])+'].in++ <-- OC12 <-- internal['+str(uid_to_index[n2])+'].out++;\n')
    f.write('}\n\n')
    f.close()

write_to_ned()

