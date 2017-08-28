import json
import requests
requesito = requests.get("http://127.0.0.1:8888/", params = {"w":"774508"})
if requisito.status_code == 200:
    print("exitoso")

d = {"sitio": "Recursos Python", "url": "recursospython.com"}
res = requests.post("http://127.0.0.1:8888/", data=json.dumps(d))
if res.status_code == 200:
    print res.text
    print res.url
    print res.headers
    print res.status_code
    print res.encoding
    print res.json
