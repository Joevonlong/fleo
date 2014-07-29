import fileinput

# user control
fanout = 4

indent = '        ' # 8 spaces
def gotoSubmoduleSection():
    for line in fileinput.input('tree.ned', inplace=1):
        print line, # comma somehow excludes \n at the end
        if 'submodules' in line:
            break

def addSubmodules():
    print indent+'beyond: Beyond;'
    print indent+'core['+str(fanout)+']: Core;'
    print indent+'pop['+str(fanout**2)+']: PoP;'
    print indent+'user['+str(fanout**3)+']: User;'

def addConnections():
    print '    connections:'
    # beyond-core
    for i in range(1): # just for consistency
        for j in range(i*fanout,(i+1)*fanout):
            print indent+'beyond.gate++ <--> OC48 <--> core['+str(j)+'].gate++;'
    # core-core (ring)
    for i in range(fanout-1):
        print indent+'core['+str(i)+'].gate++ <--> OC12 <--> core['+str(i+1)+'].gate++;'
    print indent+'core['+str(fanout-1)+'].gate++ <--> OC12 <--> core[0].gate++;'
    # core-pop
    for i in range(fanout):
        for j in range(i*fanout,(i+1)*fanout):
            print indent+'core['+str(i)+'].gate++ <--> OC3 <--> pop['+str(j)+'].gate++;'
    # pop-user
    for i in range(fanout*fanout):
        for j in range(i*fanout,(i+1)*fanout):
            print indent+'pop['+str(i)+'].gate++ <--> Cat3 <--> user['+str(j)+'].gate;'
    print '}'

gotoSubmoduleSection()
addSubmodules()
addConnections()

#i=1
#for line in fileinput.input('event_schedule.xml', inplace=1):
#    if 'receiver' not in line:
#        print line,
#    if 'event time' in line:
#        print '    <property name="event_type" type="string">ns3::FNSSEvent</property>'
#        print '    <property name="event_id" type="string">Event '+str(i)+'</property>'
#        i += 1
