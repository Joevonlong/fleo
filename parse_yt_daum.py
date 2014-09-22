import collections

MAXLENGTH = 36000

# 1. vids of same length treated as same vid: add views
# 2. put highest number of views at top of file. faster access?
# 3. lookup table?

def parse(videoID_index, length_index, views_index):
    print('parsing '+ifn)
    infile = open(ifn)
    outfile = open(ofn, 'w')
    vids = []
    ignore_count = 0

    for l in infile:
        l = l.rstrip()
        # video ID
        videoID = l.split('|')[videoID_index]
        # video length
        hms = [int(hms) for hms in l.split('|')[length_index].split(':')]
        if len(hms) == 1:
            length = hms[0]
        elif len(hms) == 2:
            length = hms[0]*60 + hms[1]
        elif len(hms) == 3:
            length = hms[0]*3600 + hms[1]*60 + hms[2]
        else:
            print('unparsed time')
        if length > MAXLENGTH:
            print('length cut from ' + str(length) + ' to ' + str(MAXLENGTH))
            length = MAXLENGTH
        elif length == 0:
            #print('ignoring video of zero length')
            ignore_count += 1
            continue
        # video view count
        views = int(l.split('|')[views_index])
        vids.append((videoID, length, views))
    print(str(ignore_count)+' videos of zero length ignored')
    # sort by descending viewcount to speed up lookups
    vids.sort(key=lambda x:x[2], reverse=True)
    # write metadata
    outfile.write(str(len(vids)) +'\t' +
                  str(sum([v[1] for v in vids])) +'\t' +
                  str(sum([v[2] for v in vids])) +'\n')
    for vid, l, v in vids:
        outfile.write(vid + '\t' + str(l) +'\t' +str(v) +'\n')

    infile.close()
    outfile.close()

# Daum Food and Travel Categories
# Format: video_id | upload_date | length | user_id | recommended | views
ifn = "yt_daum_data/Daum_Food_20070403.txt"
ofn = "freqs_daum_food.txt"
parse(0, 2, 5)
ifn = "yt_daum_data/Daum_Travel_20070412.txt"
ofn = "freqs_daum_travel.txt"
parse(0, 2, 5)
# YouTube Entertainment Category
# Format: url | length | views | ratings | stars
ifn = "yt_daum_data/YoutubeEntDec212006.txt"
ofn = "freqs_yt_ent.txt"
parse(0, 1, 2)
# YouTube Science & Technology Category
# Format: url | length | views1 | ratings1 | user_id | upload_date | views2 | comments2 | favorited2 | ratings2 | stars2 | honors2 | links2 | related2
ifn = "yt_daum_data/YoutubeSciJan162007.txt"
ofn = "freqs_yt_sci.txt"
parse(0, 1, 2)

