
import requests

data = """
{
    "view": {
	"edges": {
	    "BasicEdge": {
		"filterFunctions": [
		    {
			"function": {
			    "class": "gaffer.function.simple.filter.MapContains",
			    "key": "@r"
			},
			"selection": [
			    {
				"key": "name"
			    }
			]
		    }
		]
	    }
	}
    },
    "seeds": [
	{
	    "gaffer.operation.data.EntitySeed": {
		"vertex": "n:u:http://ex.org/#lion"
	    }
	}
    ],
    "includeIncomingOutGoing": "OUTGOING"
}
"""

url = 'http://localhost:8080/example-rest/v1/graph/doOperation/get/edges/related'

response = requests.post(url, data,
                         headers={'content-type': 'application/json'})

print "Status:",response.status_code
print response.text

