
import py4j.GatewayServer;

import gaffer.jsonserialisation.JSONSerialiser;
import gaffer.exception.SerialisationException;
import gaffer.data.element.Edge;
import gaffer.operation.impl.add.AddElements;

import gaffer.graph.Graph;
import gaffer.operation.OperationException;
import gaffer.operation.OperationChain;
import gaffer.operation.impl.get.GetRelatedEdges;
import java.io.InputStream;
import java.io.FileInputStream;
import java.io.IOException;

class GafferEntryPoint {

    private Graph graph;

    public GafferEntryPoint() throws IOException {

	try {

	    InputStream schema = new FileInputStream("schema.json");

	    InputStream storeProperties =
		new FileInputStream("store.properties");
	    
	    graph = new Graph.Builder()
		.addSchema(schema)
		.storeProperties(storeProperties)
		.build();

	} catch (IOException e) {
	    System.out.println("Exception " + e);
	    throw e;
	}

    }

    public Graph getGraph() {
	return graph;
    }

    public void addElements(String json)
	throws SerialisationException, OperationException {

	JSONSerialiser j = new JSONSerialiser();
	AddElements ae;

	ae = (AddElements) j.deserialise(json.getBytes(),
					 AddElements.class);

	graph.execute(ae);

    }

    public String getRelatedEdges(String json)
	throws SerialisationException, OperationException {

	JSONSerialiser j = new JSONSerialiser();

	GetRelatedEdges o;

	o = (GetRelatedEdges) j.deserialise(json.getBytes(),
					    GetRelatedEdges.class);

	Iterable<Edge> edges = graph.execute(o);

	byte[] result = j.serialise(edges);

	return new String(result);

    }

    public Object execute(OperationChain<?> o) throws OperationException {
	return graph.execute(o);
    }

    public String doOperation(String json)
	throws SerialisationException, OperationException {

	JSONSerialiser j = new JSONSerialiser();

	OperationChain o;

	o = (OperationChain) j.deserialise(json.getBytes(),
					   OperationChain.class);

	Object out = execute(o);

	byte[] result = j.serialise(out);

	return new String(result);

    }

    public static void main(String[] args) {
	try {
	    GatewayServer gatewayServer =
		new GatewayServer(new GafferEntryPoint());
	    gatewayServer.start();
	    System.out.println("Gateway Server Started");
	} catch (IOException e) {
	    System.out.println("Exception " + e);
	}
    }

}
