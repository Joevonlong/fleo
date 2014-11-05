"""
Demo of the histogram (hist) function with a few features.

In addition to the basic histogram, this demo shows a few optional features:

    * Setting the number of data bins
    * The ``normed`` flag, which normalizes bin heights so that the integral of
      the histogram is 1. The resulting histogram is a probability density.
    * Setting the face color of the bars
    * Setting the opacity (alpha value).

"""
import numpy as np
import matplotlib.mlab as mlab
import matplotlib.pyplot as plt

scafn = 'results/AS1221-0.sca'
vecfn = 'results/AS1221-0.vec'

# plot hops to reach content
def plot_hopcount():
    vecf = open(vecfn)
    # find vector identifier for hop count
    for l in vecf:
        if 'hops vector' in l:
            hops_ID = l.split()[1]
            break
    # look for identifier at start of subsequent lines
    hop_counts = []
    for l in vecf:
        if l.startswith(hops_ID):
            hop_counts.append(int(l.split()[3]))
    # plot histogram of hop counts
    n, bins, patches = plt.hist(hop_counts,
                                bins=max(hop_counts),
                                range=(min(hop_counts), max(hop_counts)+1),
                                align='left',
                                rwidth=1,
                                #facecolor='green', alpha=0.5
                                )
    plt.xlabel('Hop count')
    plt.ylabel('Number of requests')
    plt.title(r'Number of hops for request to find cached content')

    # Tweak spacing to prevent clipping of ylabel
    #plt.subplots_adjust(left=0.15)
    plt.grid()
    plt.show()
    vecf.close()

# plot distance from user to nearest cache
def plot_dist_to_cache():
    scaf = open(scafn)
    # find section for dist-to-cache
    for l in scaf:
        if 'distance to cache' in l:
            break
    # skip min, max etc...
    for l in scaf:
        if not l.startswith('field'):
            break
    # read bins
    lowers = []
    freqs = []
    # remember first line is already read after previous loop
    assert l.startswith('bin\t-INF\t0')
    for l in scaf:
        if l.startswith('bin'):
            [_, lower, freq] = l.split()
            if freq == '0':
                continue
            else:
                lowers.append(int(np.ceil(float(lower))))
                freqs.append(int(freq))
                continue
        else:
            break
    # plot histogram
    print(lowers)
    print(freqs)
    n, bins, patches = plt.hist(lowers,
                                bins=max(lowers),
                                range=(min(lowers), max(lowers)+1),
                                weights=freqs,
                                align='left',
                                rwidth=1,
                                #facecolor='green', alpha=0.5
                                )
    plt.xlabel('Hops to nearest cache')
    plt.ylabel('Number of users')
    plt.title(r'Histogram of user hops to nearest cache')
    plt.grid()
    plt.show()
    scaf.close()

# plot histogram of startup delays
def plot_startup_delay():
    vecf = open(vecfn)
    # find vector identifiers
    long_ID = short_ID = None
    while (short_ID == None) or (long_ID == None):
        l = vecf.readline()
        if 'startup delay vector' in l:
            long_ID = l.split()[1]
        elif 'startup delay for short videos vector' in l:
            short_ID = l.split()[1]
    # look for identifiers at start of subsequent lines
    long_delays = []
    short_delays = []
    for l in vecf:
        if l.startswith(long_ID):
            long_delays.append(float(l.split()[3]))
        elif l.startswith(short_ID):
            short_delays.append(float(l.split()[3]))
    #delays = [for p in zip(long_delays, short_]
    # plot stacked histogram of startup delays
    n, bins, patches = plt.hist([short_delays, long_delays],
                                bins=100,
                                range=(0, max(max(long_delays),max(short_delays))),
                                histtype='barstacked',
                                #align='left',
                                #rwidth=1,
                                log=True,
                                #facecolor='green', alpha=0.5,
                                label=['length < 20s', 'length >= 20s'],
                                )
    plt.legend(prop={'size': 10})
    plt.xlabel('Startup delay / s')
    plt.ylabel('Number of views')
    plt.title(r'Histogram of startup delays')
    plt.grid()
    plt.show()
    vecf.close()

# plot moving average of startup delays
def plot_delayavg():
    vecf = open(vecfn)
    # find vector identifiers
    long_ID = short_ID = None
    while (short_ID == None) or (long_ID == None):
        l = vecf.readline()
        if 'startup delay vector' in l:
            long_ID = l.split()[1]
        elif 'startup delay for short videos vector' in l:
            short_ID = l.split()[1]
    # look for identifiers at start of subsequent lines
    delays = []
    delay_avgs = []
    counter = 0
    for l in vecf:
        if l.startswith(long_ID) or l.startswith(short_ID):
            delays.append(float(l.split()[3]))
            counter += 1
            if counter == 1000:
                delay_avgs.append(sum(delays)/1000.0)
                delays = []
                counter = 0
    # plot moving average
    plt.plot(delay_avgs)
    plt.xlabel('Request index / 1000')
    plt.ylabel('Average Startup delay / s')
    plt.title(r'Window averages of startup delays')
    plt.grid()
    plt.ylim(ymin=0.0)
    plt.show()
    vecf.close()

# plot histogram of video lengths
def plot_vidlen():
    ffn = 'freqs_yt_sci.txt'
    ff = open(ffn)
    # skip first line
    ff.readline()
    # read subsequent lines
    len_views = {}
    lv=0
    for l in ff:
        length = int(l.split()[1])
        views = int(l.split()[2])
        if length in len_views:
            len_views[length] += views
        else:
            len_views[length] = views
        lv += length*views
    n, bins, patches = plt.hist(len_views.keys(),
                                bins=np.logspace(0, 5, 200),
                                normed=True,
                                weights=len_views.values(),
                                cumulative=True,
                                #log=True,
                                #facecolor='green', alpha=0.5
                                )
    plt.gca().set_xscale("log")
    plt.xlabel('Video length')
    plt.ylabel('Cumulative fraction of views')
    plt.title(r'Distribution of video lengths by number of views')
    plt.grid()
    plt.xlim(0,36000)
    plt.ylim(0,1)
    plt.show()
    ff.close()

if __name__ == '__main__':
    print('1. Histogram of hop counts to reach cached item')
    print('2. Histogram of video lengths')
    print('3. Histogram of user-to-nearest-cache distances')
    print('4. Histogram of startup delays')
    choice = input('Which to plot? ')
    if choice == 1:
        plot_hopcount()
    elif choice == 2:
        plot_vidlen()
    elif choice == 3:
        plot_dist_to_cache()
    elif choice == 4:
        plot_startup_delay()
    elif choice == 5:
        plot_delayavg()
