import collections

# 1. vids of same length treated as same vid: add views
# 2. put highest number of views at top of file. faster access?
# 3. lookup table?

# Daum Food and Travel Categories
# Format: video_id | upload_date | length | user_id | recommended | views
def parse_daum():
    infile = open(ifn)
    outfile = open(ofn, 'w')
    count = collections.Counter()

    for l in infile:
        length = int(l.split('|')[2])
        views = int(l.split('|')[5][:-1])
        count[length] += views

    outfile.write(str(len(count)) +'\t' +
                  str(sum(count.keys())) +'\t' +
                  str(sum(count.values())) +'\n')
    for l,v in count.most_common():
        outfile.write(str(l) +'\t' +str(v) +'\n')

    infile.close()
    outfile.close()

ifn = "yt_daum_data/Daum_Food_20070403.txt"
ofn = "daum_food_freqs.txt"
parse_daum()
ifn = "yt_daum_data/Daum_Travel_20070412.txt"
ofn = "daum_travel_freqs.txt"
parse_daum()

# YouTube Entertainment Category
# Format: url | length | views | ratings | stars
ifn = "yt_daum_data/YoutubeEntDec212006.txt"
ofn = "yt_ent_freqs.txt"
infile = open(ifn)
outfile = open(ofn, 'w')
count = collections.Counter()
for l in infile:
    length = int(l.split('|')[1].split(':')[0])*60 +\
             int(l.split('|')[1].split(':')[1])
    views = int(l.split('|')[2])
    count[length] += views
outfile.write(str(len(count)) +'\t' +
              str(sum(count.keys())) +'\t' +
              str(sum(count.values())) +'\n')
for l,v in count.most_common():
    outfile.write(str(l) +'\t' +str(v) +'\n')
infile.close()
outfile.close()

# YouTube Science & Technology Category
# Format: url | length | views1 | ratings1 | user_id | upload_date | views2 | comments2 | favorited2 | ratings2 | stars2 | honors2 | links2 | related2
ifn = "yt_daum_data/YoutubeSciJan162007.txt"
ofn = "yt_sci_freqs.txt"
infile = open(ifn)
outfile = open(ofn, 'w')
count = collections.Counter()
for l in infile:
    length = int(l.split('|')[1].split(':')[0])*60 +\
             int(l.split('|')[1].split(':')[1])
    views = int(l.split('|')[2])
    count[length] += views
outfile.write(str(len(count)) +'\t' +
              str(sum(count.keys())) +'\t' +
              str(sum(count.values())) +'\n')
for l,v in count.most_common():
    outfile.write(str(l) +'\t' +str(v) +'\n')
infile.close()
outfile.close()

