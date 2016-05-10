import urllib2

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

req = urllib2.Request('http://localhost:8080/graph/doOperation')
response = urllib2.urlopen(req, data)
result = response.read()

print result

