import pymysql
conn = pymysql.connect(host='주소',
                       user="계정ID",
                       password="계정PW",
                       db="접근DB",
                       charset="utf8"
                      )

cur = conn.cursor()
sql = 'SELECT * FROM test'

cur.execute(sql)
results = cur.fetchall()

print(results)

conn.close()