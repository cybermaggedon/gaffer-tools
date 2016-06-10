import requests

data = """
{
    "operations": [
	{
	    "class": "gaffer.accumulostore.operation.impl.GetEdgesInRanges",
	    "seeds": [
		{
		    "gaffer.accumulostore.utils.Pair": {
			"first": {
			    "gaffer.operation.data.EntitySeed": {
				"vertex": "n:"
			    }
			},
			"second": {
			    "gaffer.operation.data.EntitySeed": {
				"vertex": "n;"
			    }
			}
		    }
		}
	    ],
	    "includeIncomingOutGoing": "INCOMING"
	}
    ]
}
"""

url = 'http://localhost:8080/example-rest/v1/graph/doOperation'
response = requests.post(url, data,
                         headers={'content-type': 'application/json'})

print "Status:",response.status_code
print response.text


