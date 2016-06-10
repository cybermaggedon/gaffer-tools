import requests

data = """
{
    "elements": [
	{
	    "class": "gaffer.data.element.Edge",
	    "properties": {
		"name": {
  		    "gaffer.function.simple.types.FreqMap": {
                         "r:u:http://ex.org/#eats": 1,
                         "@r": 1
                    } 
                }
	    },
	    "group": "BasicEdge",
	    "source": "n:u:http://ex.org/#lion",
	    "destination": "n:u:http://ex.org/#zebra",
	    "directed": true
	},
	{
	    "class": "gaffer.data.element.Edge",
	    "properties": {
		"name": {
  		    "gaffer.function.simple.types.FreqMap": {
			"n:u:http://ex.org/#zebra": 1,
			"@n": 1
		    }
		}
	    },
	    "group": "BasicEdge",
	    "source": "n:u:http://ex.org/#lion",
	    "destination": "r:u:http://ex.org/#eats",
	    "directed": true
	}
    ]
}
"""

url = "http://localhost:8080/example-rest/v1/graph/doOperation/add/elements"

response = requests.put(url, data, headers={'content-type':'application/json'})

print "Status:",response.status_code
print response.text

