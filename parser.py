import json

data = []

query = "insert into intrebari values ({}, '{}', {}, '{}', '{}', '{}');"

with open("test.txt", "r") as f:
    content = f.read().split("\n")
    lines = len(content)
    for q_nr in range(lines//7+1):
        data.append(query.format(
            content[q_nr*7],
            content[q_nr*7+1],
            content[q_nr*7+2],
            content[q_nr*7+3],
            content[q_nr*7+4],
            content[q_nr*7+5]
            ))
with open("test.sql", "w") as f:
    f.write("delete from intrebari;\n\n")
    f.write("\n".join(data))