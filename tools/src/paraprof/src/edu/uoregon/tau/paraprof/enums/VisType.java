package edu.uoregon.tau.paraprof.enums;

/**
 * type-safe enum pattern for visualization type
 *    
 * TODO : nothing, this class is complete
 *
 * <P>CVS $Id: VisType.java,v 1.2 2005/05/31 23:21:50 amorris Exp $</P>
 * @author  Alan Morris
 * @version $Revision: 1.2 $
 */
public class VisType {

    private final String name;
    private VisType(String name) { this.name = name; }
    public String toString() { return name; }
    
    public static final VisType TRIANGLE_MESH_PLOT = new VisType("Triangle Mesh");
    public static final VisType BAR_PLOT = new VisType("Bar Plot");
    public static final VisType SCATTER_PLOT = new VisType("Scatter Plot");
    //public static final VisType KIVIAT_TUBE = new VisType("Kiviat Tube");
    
    
    
}
