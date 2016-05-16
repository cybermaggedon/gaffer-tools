import urllib2

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

req = urllib2.Request('http://localhost:8080/graph/doOperation/get/edges/related')
response = urllib2.urlopen(req, data)
result = response.read()

print result

