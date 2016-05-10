
# Getting tooled up to use Gaffer

This page describes a set of alpha-quality tools I'm playing with to get working
with the Gaffer graph database.

## Compiling Gaffer

This is covered in
[Gaffer's Getting Started Guide](https://github.com/GovernmentCommunicationsHeadquarters/Gaffer/wiki/Getting-Started).  But basically, download the source and run:

```
mvn clean package
```

## The REST interface

### Starting the REST interface

Gaffer's REST interface is mostly there, it just needs some packaging work, and
this is happening on a development branch.  In the interim I've got a simple
REST interface by embedding Gaffer into a Python web server using py4j.  This
implements a small subset of the REST API.  It uses Gaffer's JSON serialisers
and deserialisers, so should be interface-compatible with the REST API when
it is released.

This is not a production solution, and it won't perform or scale: The Python
aseHTTPServer class is single-threaded.

This is in the `rest-api` directory.  Components are:
* The `GafferEntryPoint` Java class instantiates the Gaffer store.  It is invoked through *py4j*.
* A Python wrapper which hosts the web-server and invokes Gaffer operations through the *py4j* interface.
* The `schema.json` configuration file defines the Gaffer schema.  It contains a configuration we'll use to store RDF later.
* The `store.properties` configuration file defines the Gaffer store.  This is configured to run a `MockAccumuloStore`, but changing this configuration would permit using a real Accumulo store.

To get this up and running:

1. First step is to assemble a `CLASSPATH` variable.  If it's any help, the 
   `env` file I've put in the directory will work for you.  If not, you're
   on your own.
   ```
   . ./env
   ```

2. Compile the `GafferEntryPoint` class:
   ```
   javac GafferEntryPoint.java
   ```

3. Run the `GafferEntryPoint` class:
   ```
   java GafferEntryPoint
   ```
   If it works, it doesn't return. This starts the Gaffer storage in that Java process.  You may get a warning about Hadoop NativeCodeLoader not working.  Seems safe to ignore this warning.

4. Run the Python service wrapper:
   ```
   ./service
   ```
   
   If both of those are running, you have a Gaffer with a REST API on port 8080.

### Testing the REST interface

There are three Python scripts which test the REST API is working:

* `python addelements.py`.  It adds some elements to the graph.  It produces no output.
* `python dooperations.py` runs a key range query.  Output should be this:
   
   ```
   [{"class":"gaffer.data.element.Edge","properties":{"name":{"gaffer.function.simple.types.FreqMap":{"r:http://ex.org/#eats":4,"@r":4}}},"group":"BasicEdge","source":"n:http://ex.org/#lion","destination":"n:http://ex.org/#zebra","directed":true}]
   ```

* `python getrelatededges.py` runs a related edge query.  Output should look like this:
   ```
   [{"class":"gaffer.data.element.Edge","properties":{"name":{"gaffer.function.simple.types.FreqMap":{"r:http://ex.org/#eats":4,"@r":4}}},"group":"BasicEdge","source":"n:http://ex.org/#lion","destination":"n:http://ex.org/#zebra","directed":true}]
   ```

The Gaffer code is Java code.  You know if there's a problem, because you get a massive stack trace.

## Redland, RDF and SPARQL

### Install Redland

[Redland](http://librdf.org) is a set of APIs and utilities which support storage of RDF.  You'll need to install Redland and the development libraries.  On Fedora, you need to install this set of packages:
* `redland`
* `redland-devel`
* `raptor2`
* `raptor2-devel`
* `rasqal`
* `rasqal-devel`
* `librdf`
* `librdf-devel`
* `python-librdf`

### Compiling the code

This is in the `redland` directory.   Once Redland is installed, you should be able to compile the code by typing:
```make```
from the `redland` directory.

This compiles a set of things:
* A Redland store for Gaffer, `librdf_storage_gaffer.so`.  This needs to be installed in the Redland storage module directory, which on my box is `/usr/share/redland`.
* An executable, `test-sqlite` which performs a set of RDF operations against an SQLite store `STORE.DB`.
* An executable, `test-gaffer` which performs the same set of RDF operations
against a Gaffer database.

Before doing anything complicated, your first step is to check that the SQLite
code works:
```
./test-sqlite
```
You see a bunch of output, of which the last line is:
```
** Remove statements
```

If that works, the next step is to install the Gaffer storage module.  On my machine, the Redland storage modules are in `/usr/lib64/redland`.  If that's a directory on your machine, you can just run `make install` to install the module to that directory.

If that works, Redland now knows about accessing Gaffer through the REST API.  Try:
```
./test-gaffer
```
to see if you get the same output.

### Playing with Redland

If you got this far without errors, you have a Gaffer store running, with a few edges in it.  Redland has a set of command line tools you can use.  For instance:

You can dump out the edges:
```
rdfproc -s gaffer http://localhost:8080/ print
```
And run a SPARQL query:
```
rdfproc -s gaffer http://localhost:8080/ query sparql - 'SELECT ?a ?b ?c WHERE { ?a ?b ?c . }'
```

A more complex query:
```
rdfproc -s gaffer http://localhost:8080/ query sparql - 'SELECT ?a WHERE { <http://gaffer.test/number#3>  <http://gaffer.test/number#is_before> ?b . ?b <http://gaffer.test/number#is_before> ?a . }'
```

If you have some RDF lying around, you can load it into the graph:
```
rdfproc -n -s gaffer localhost parse my_data.rdf
```

### Redland Python API

Redland has a set of language bindings, [including the Python binding](http://librdf.org/docs/pydoc/RDF.html).  This is contained in the Fedora `python-librdf` package.

```python
# Add a statement
node1=RDF.Node(uri_string="http://ex.org/#lion")
node2=RDF.Node(uri_string="http://ex.org/#name")
node3=RDF.Node(literal="Lion")
statement=RDF.Statement(node1, node2, node3)
model.add_statement(statement)

# Match on partial statement
statement=RDF.Statement(node1, None, None)
statements = model.find_statements(statement)
for v in statements:
    print v

# Run a SPARQL query
query = RDF.SPARQLQuery("SELECT ?b ?c WHERE { <http://ex.org/#lion> ?b ?c . }")
results = model.execute(query)
print results
```

## SPARQL endpoint

### Running the service

SPARQL defines a protocol which can be used to execute SPARQL queries remotely.  This is interesting for us, because a number of products know how to use a SPARQL endpoint and would be easy to integrate.

Now we have RDF in Python, a bit of Python easily provides the SPARQL endpoint.  Look in the `sparql` directory, and run:
```
./sparql_service
```
This runs a SPARQL service on `http://localhost:8081/sparql`.

### Interacting with the SPARQL endpoint

Redland's `roqet` utility knows how to query SPARQL. Example usage:
```
roqet -p http://localhost:8081/sparql -e 'SELECT ?a ?b ?c WHERE { ?a ?b ?c . }'
```
