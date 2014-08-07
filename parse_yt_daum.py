import collections

daum_food = open("yt_daum_data/Daum_Food_20070403.txt")
# Format: video_id | upload_date | length | user_id | recommended | views

output = open("daum_food_freqs.txt", 'w')

# 1. vids of same length treated as same vid: add views
# 2. put highest number of views at top of file. faster access?
# 3. lookup table?

count = collections.Counter()

for l in daum_food:
    length = int(l.split('|')[2])
    views = int(l.split('|')[5][:-1])
    count[length] += views

output.write(str(len(count)) +'\t' +
             str(sum(count.keys())) +'\t' +
             str(sum(count.values())) +'\n')
#output.write('length\tviews\n')
for l,v in count.most_common():
    output.write(str(l) +'\t' +str(v) +'\n')

daum_food.close()
output.close()

