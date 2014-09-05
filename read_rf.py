import fileinput
import tarfile
import re
import collections

# global vars
fn = "rocketfuel_maps_cch.tar.gz"
archive = tarfile.open(fn)
asn = '1221'
file0 = archive.extractfile(asn+'.cch')
num_replica = 15

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
            pass
            #print("len(nuids) != num_neigh")
            #raise ValueError
        self.euids = [int(euid) for euid in euids]
        if len(self.euids) != self.ext:
            pass
            #raise ValueError(self.uid, self.euids, self.ext)
        self.name = name
        self.alias = True if alias=='!' else False
        self.rn = int(rn)
        self.assignment = None

class Ext:
    def __init__(self, euid, address, rn):
        self.euid = euid
        self.address = address
        self.rn = rn

class AS:
    def __init__(self):
        self.nodes = []
        self.exts = []
        self.uids = {}
        self.links = set()
        self.link_count = 0

    def add_node(self, node):
        self.nodes.append(node)
        self.uids[node.uid] = node
        self.link_count += node.num_neigh + node.ext # counts both ways
        uid = node.uid
        # avoid adding reverse of existing tuple/link
        for nuid in node.nuids:
            if (nuid,uid) not in self.links:
                self.links.add((uid,nuid))

    # first seen if more than 1
    def get_most_connected(self):
        max_neigh = 0
        most_conn_node = None
        for node in self.nodes:
            if node.num_neigh > max_neigh:
                max_neigh = node.num_neigh
                most_conn_node = node
        print('node '+str(most_conn_node.uid)+' is the most connected.')
        self.most_connected = most_conn_node
        return most_conn_node

    def prune_unreachable_from(self, uid):
        if isinstance(uid, Node):
            uid = uid.uid
        reachable_uids = set([uid])
        reachable_links = set()
        prev_num_reachable = -1
        print('Pruning unreachable nodes...')
        while prev_num_reachable != len(reachable_uids):
            prev_num_reachable = len(reachable_uids)
            print(str(prev_num_reachable)+' nodes reachable')
            for n1,n2 in self.links:
                if n1 in reachable_uids:
                    reachable_uids.add(n2)
                    reachable_links.add((n1,n2))
                elif n2 in reachable_uids:
                    reachable_uids.add(n1)
                    reachable_links.add((n1,n2))
        print('No more reachable nodes: ' +
              str(len(reachable_uids)) + ' nodes reached')
        self.nodes = [self.uids[uid] for uid in reachable_uids]
        self.links = reachable_links

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
                    r'(&(?P<ext>\d+) )?->\s+'
                    r'(<(?P<nuid>[\d<> ]+)> )?'
                    r'(\{(?P<euid>[-\d{} ]+)\} )?\s?'
                    r'=(?P<name_alias>\S+) '
                    r'r(?P<rn>\d+)'
                    )

euid_re = re.compile(r'-(?P<euid>\d+)\s+'
                     r'=(?P<address>\S+) '
                     r'r(?P<rn>\d+)'
                     )

asys = AS()
exts = []
def parse():
    for line in file0:
        m = uid_re.match(line.decode('UTF-8'))
        if m != None:
            if m.group('nuid'):
                neigh_uids = [int(x) for x in m.group('nuid').split('> <')]
            else:
                neigh_uids = []
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
            asys.add_node(
                Node(int(m.group('uid')), m.group('loc'), m.group('dns'),
                     m.group('bb'), int(m.group('num_neigh')), m.group('ext'),
                     neigh_uids, ext_uids, name, alias, int(m.group('rn'))
                     )
                )
            continue
        m = euid_re.match(line.decode('UTF-8'))
        if m != None:
            exts.append(
                Ext(int(m.group('euid')), m.group('address'), int(m.group('rn')))
                )
            continue
        print('unrecognised line:')
        print(line)
    print('parsed '+str(len(asys.nodes))+' internal nodes')
    print('parsed '+str(len(exts))+' external nodes')

def manip_topo():
    # take most connected node to be server
    asys.get_most_connected().assignment = 'beyond'

    # prune nodes and links unable to reach server
    asys.prune_unreachable_from(asys.most_connected)
    print(str(len(asys.nodes))+' internal nodes after pruning unreachables')

    # singly connected nodes will be users
    user_count = 0
    for node in asys.nodes:
        if node.num_neigh == 1:
            node.assignment = 'user'
            user_count += 1
    print(str(user_count)+' nodes assigned as users')

    # place a replica in descending order of cities/locs with most users
    # - find candidate replica positions (most connected nodes)
    loc_centres = {}
    for n in asys.nodes:
        if n.loc in loc_centres.keys():
            if n.num_neigh > loc_centres[n.loc].num_neigh:
                loc_centres[n.loc] = n
        else:
            loc_centres[n.loc] = n
    # - order cities/locs by number of users
    loc_users = collections.Counter()
    for n in asys.nodes:
        if n.assignment == 'user':
            loc_users[n.loc] += 1
    # - place cache in centres of locs with most users
    for (loc, users) in loc_users.most_common()[:num_replica]:
        loc_centres[loc].has_cache = True
        print('cache assigned to UID '+str(loc_centres[loc].uid)+
              ' for '+loc+' ('+str(users)+' users)')

def write_to_ned():
    f = open('as'+asn+'.ned', 'w')
    f.write('network AS'+asn.replace('.','')+'\n{\n')

    # submodule section
    f.write(' '*4+'submodules:\n')
    f.write(' '*8+'global: Global;\n')
    for node in asys.nodes:
        if node.assignment == 'beyond':
            f.write(' '*8+'beyond'+str(node.uid)+': Beyond{name="'
                    +node.name+'"; rn='+str(node.rn)+';')
        elif node.assignment == 'user':
            f.write(' '*8+'user'+str(node.uid)+': User{name="'
                    +node.name+'"; rn='+str(node.rn)+';')
        elif node.bb:
            f.write(' '*8+'core'+str(node.uid)+': Core{name="'
                    +node.name+'"; rn='+str(node.rn)+';')
        else:
            f.write(' '*8+'access'+str(node.uid)+': PoP{name="'
                    +node.name+'"; rn='+str(node.rn)+';')
        if (getattr(node, 'has_cache', False) == True):
            f.write(' hasCache=true; @display("i=block/routing,green");')
        f.write('};\n')

    # connection section
    f.write(' '*4+'connections:\n')
    for (n1,n2) in asys.links:
        if asys.uids[n1].bb:
            left = 'core'
        else: # 'access'
            left = 'access'
        if asys.uids[n2].bb:
            right = 'core'
        else: # 'access'
            right = 'access'
        # override if beyond
        if asys.uids[n1].assignment == 'beyond':
            left = 'beyond'
        if asys.uids[n2].assignment == 'beyond':
            right = 'beyond'
        # override if user
        if asys.uids[n1].assignment == 'user':
            f.write(' '*8+'user'+str(n1)+'.out --> OC12 --> '
                    +right+str(n2)+'.in++;\n')
            f.write(' '*8+'user'+str(n1)+'.in <-- OC12 <-- '
                    +right+str(n2)+'.out++;\n')
            continue
        if asys.uids[n2].assignment == 'user':
            f.write(' '*8+left+str(n1)+'.out++ --> OC12 --> '
                    +'user'+str(n2)+'.in;\n')
            f.write(' '*8+left+str(n1)+'.in++ <-- OC12 <-- '
                    +'user'+str(n2)+'.out;\n')
            continue
        f.write(' '*8+left+str(n1)+'.out++ --> OC12 --> '
                +right+str(n2)+'.in++;\n')
        f.write(' '*8+left+str(n1)+'.in++ <-- OC12 <-- '
                +right+str(n2)+'.out++;\n')

    f.write('}\n\n')
    f.close()

def main():
    parse()
    manip_topo()
    write_to_ned()

if __name__ == '__main__':
    main()

