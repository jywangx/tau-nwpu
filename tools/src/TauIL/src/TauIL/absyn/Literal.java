package TauIL.absyn;

public class Literal implements SyntaxAttribute {
    public Double value;

    public Literal(Double value) {
	this.value = value;
    }

    public Literal(double value) {
	this.value = new Double(value);
    }

    public String generateSyntax() {
	String syntax = "" + value;

	return syntax;
    }
}
