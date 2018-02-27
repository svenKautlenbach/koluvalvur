import codecs
import json
import sys

try:
    file_name = sys.argv[1]
    with codecs.open(file_name, "r", encoding='utf-8', errors='ignore') as f:
        for line in f:
            start = line.find("{\"data\":")
            if start == -1:
                continue
            end = line.find("}")
            if end == -1:
                continue
            eventData = line[start:end + 1]
            j = json.loads(eventData)
            print(j)

    f.close()
except ValueError as e:
    print("Caught ValueError:")
    print(e)
except Exception as e:
    print("Caught general exception:")
    print(e)

