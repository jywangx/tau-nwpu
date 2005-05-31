package edu.uoregon.tau.paraprof;

import java.awt.*;
import java.awt.event.InputEvent;
import java.awt.event.MouseEvent;
import java.awt.event.MouseListener;
import java.awt.font.*;
import java.awt.print.PageFormat;
import java.awt.print.Printable;
import java.text.AttributedCharacterIterator;
import java.text.AttributedString;
import java.util.ArrayList;
import java.util.List;
import java.util.StringTokenizer;

import javax.swing.JPanel;
import javax.swing.JPopupMenu;

import edu.uoregon.tau.dms.dss.Function;
import edu.uoregon.tau.dms.dss.UtilFncs;
import edu.uoregon.tau.paraprof.enums.ValueType;
import edu.uoregon.tau.paraprof.interfaces.ImageExport;

/**
 * FunctionDataWindowPanel
 * This is the panel for the FunctionDataWindow.
 *  
 * <P>CVS $Id: FunctionDataWindowPanel.java,v 1.18 2005/05/31 23:21:48 amorris Exp $</P>
 * @author	Robert Bell, Alan Morris
 * @version	$Revision: 1.18 $
 * @see		FunctionDataWindow
 */
public class FunctionDataWindowPanel extends JPanel implements MouseListener, Printable, ImageExport {

    private ParaProfTrial ppTrial = null;
    private FunctionDataWindow window = null;
    private List list = new ArrayList();
    private Function function = null;

    //Drawing information.
    private int barHeight = -1;
    private int barSpacing = -1;
    private int baseBarLength = 250;
    private int barLength = 0;
    private int textOffset = 60;
    private int barXCoord = 0;
    private int lastHeaderEndPosition = 0;

    //Panel information.
    private int xPanelSize = 0;
    private int yPanelSize = 0;

    public FunctionDataWindowPanel(ParaProfTrial ppTrial, Function function, FunctionDataWindow window) {

        this.ppTrial = ppTrial;
        this.window = window;
        this.function = function;
        barLength = baseBarLength;

        setBackground(Color.white);

        addMouseListener(this);

    }

    public void paintComponent(Graphics g) {
        try {
            super.paintComponent(g);
            export((Graphics2D) g, true, false, false);
        } catch (Exception e) {
            ParaProfUtils.handleException(e);
            window.closeThisWindow();
        }
    }

    public int print(Graphics g, PageFormat pageFormat, int page) {
        try {
            if (page >= 1) {
                return NO_SUCH_PAGE;
            }

            ParaProfUtils.scaleForPrint(g, pageFormat, xPanelSize, yPanelSize);
            export((Graphics2D) g, false, true, false);

            return Printable.PAGE_EXISTS;
        } catch (Exception e) {
            new ParaProfErrorDialog(e);
            return NO_SUCH_PAGE;
        }
    }

    public void export(Graphics2D g2D, boolean toScreen, boolean fullWindow, boolean drawHeader) {

        list = window.getData();

        int stringWidth = 0;
        int yCoord = 0;
        barXCoord = barLength + textOffset;

        //To make sure the bar details are set, this
        //method must be called.
        ppTrial.getPreferencesWindow().setBarDetails(g2D);

        //Now safe to grab spacing and bar heights.
        barSpacing = ppTrial.getPreferencesWindow().getBarSpacing();
        barHeight = ppTrial.getPreferencesWindow().getBarHeight();

        //Obtain the font and its metrics.
        Font font = new Font(ppTrial.getPreferencesWindow().getParaProfFont(),
                ppTrial.getPreferencesWindow().getFontStyle(), barHeight);
        g2D.setFont(font);
        FontMetrics fmFont = g2D.getFontMetrics(font);

        //Get the max value for this function

        //        double maxValue = ParaProfUtils.getMaxValue(function, window.getValueType(), window.isPercent(), ppTrial);
        double maxValue = window.getMaxValue();

        // too bad these next few lines are bullshit 
        // (you can't determine the max width by looking at the max value)  1.0E99 > 43.34534, but is thinner
        if (window.isPercent()) {
            stringWidth = fmFont.stringWidth(UtilFncs.getOutputString(0, maxValue, 6) + "%");
            barXCoord = barXCoord + stringWidth;
        } else {
            stringWidth = fmFont.stringWidth(UtilFncs.getOutputString(window.units(), maxValue,
                    ParaProf.defaultNumberPrecision));
            barXCoord = barXCoord + stringWidth;
        }

        // At this point we can determine the size this panel will require. 
        // If we need to resize, don't do any more drawing, just call revalidate.
        if (resizePanel(fmFont, barXCoord) && toScreen) {
            this.revalidate();
            return;
        }

        // determine which elements to draw (clipping)
        int[] clips = ParaProfUtils.computeClipping(g2D.getClipBounds(), window.getViewRect(), toScreen, fullWindow, list.size(), barSpacing, yCoord);
        int startElement = clips[0];
        int endElement = clips[1];
        yCoord = clips[2];

     

        //Check for group membership.
        boolean groupMember = function.isGroupMember(ppTrial.getHighlightedGroup());

        //Draw the header if required.
        if (drawHeader) {
            FontRenderContext frc = g2D.getFontRenderContext();
            Insets insets = this.getInsets();
            String headerString = window.getHeaderString();
            //Need to split the string up into its separate lines.
            StringTokenizer st = new StringTokenizer(headerString, "'\n'");
            while (st.hasMoreTokens()) {
                AttributedString as = new AttributedString(st.nextToken());
                as.addAttribute(TextAttribute.FONT, font);
                AttributedCharacterIterator aci = as.getIterator();
                LineBreakMeasurer lbm = new LineBreakMeasurer(aci, frc);
                float wrappingWidth = this.getSize().width - insets.left - insets.right;
                float x = insets.left;
                float y = insets.right;
                while (lbm.getPosition() < aci.getEndIndex()) {
                    TextLayout textLayout = lbm.nextLayout(wrappingWidth);
                    yCoord += barSpacing;
                    textLayout.draw(g2D, x, yCoord);
                    x = insets.left;
                }
            }
            yCoord = yCoord + (barSpacing);
            lastHeaderEndPosition = yCoord;
        }

        // Iterate through and draw each thread's values
        for (int i = startElement; i <= endElement; i++) {
            PPFunctionProfile ppFunctionProfile = (PPFunctionProfile) list.get(i);
            //double value = ParaProfUtils.getValue(ppFunctionProfile, window.getValueType(), window.isPercent());

            double value = ppFunctionProfile.getValue();

            yCoord = yCoord + (barSpacing);

            String barString;
            if (ppFunctionProfile.getNodeID() == -1) {
                barString = "mean";
            } else {
                barString = "n,c,t " + (ppFunctionProfile.getNodeID()) + "," + (ppFunctionProfile.getContextID()) + ","
                        + (ppFunctionProfile.getThreadID());
            }

            drawBar(g2D, fmFont, value, maxValue, barString, barXCoord, yCoord, barHeight, groupMember);
        }
    }

    private void drawBar(Graphics2D g2D, FontMetrics fmFont, double value, double maxValue, String text, int barXCoord,
            int yCoord, int barHeight, boolean groupMember) {
        int xLength = 0;
        double d = 0.0;
        String s = null;
        int stringWidth = 0;
        int stringStart = 0;
        d = (value / maxValue);
        xLength = (int) (d * barLength);
        if (xLength == 0)
            xLength = 1;

        if ((xLength > 2) && (barHeight > 2)) {
            g2D.setColor(function.getColor());
            g2D.fillRect(barXCoord - xLength + 1, (yCoord - barHeight) + 1, xLength - 1, barHeight - 1);

            if (function == (ppTrial.getHighlightedFunction())) {
                g2D.setColor(ppTrial.getColorChooser().getHighlightColor());
                g2D.drawRect(barXCoord - xLength, (yCoord - barHeight), xLength, barHeight);
                g2D.drawRect(barXCoord - xLength + 1, (yCoord - barHeight) + 1, xLength - 2, barHeight - 2);
            } else if (groupMember) {
                g2D.setColor(ppTrial.getColorChooser().getGroupHighlightColor());
                g2D.drawRect(barXCoord - xLength, (yCoord - barHeight), xLength, barHeight);
                g2D.drawRect(barXCoord - xLength + 1, (yCoord - barHeight) + 1, xLength - 2, barHeight - 2);
            } else {
                g2D.setColor(Color.black);
                g2D.drawRect(barXCoord - xLength, (yCoord - barHeight), xLength, barHeight);
            }
        } else {
            if (function == (ppTrial.getHighlightedFunction()))
                g2D.setColor(ppTrial.getColorChooser().getHighlightColor());
            else if ((function.isGroupMember(ppTrial.getHighlightedGroup())))
                g2D.setColor(ppTrial.getColorChooser().getGroupHighlightColor());
            else {
                g2D.setColor(function.getColor());
            }
            g2D.fillRect((barXCoord - xLength), (yCoord - barHeight), xLength, barHeight);
        }

        //Draw the value next to the bar.
        g2D.setColor(Color.black);
        //Do not want to put a percent sign after the bar if we are not
        // exclusive or inclusive.

        if (window.getDataSorter().getValueType() == ValueType.EXCLUSIVE_PERCENT
                || window.getDataSorter().getValueType() == ValueType.INCLUSIVE_PERCENT) {

            //s = (UtilFncs.adjustDoublePresision(value, 4)) + "%";
            s = UtilFncs.getOutputString(0, value, 6) + "%";

        } else
            s = UtilFncs.getOutputString(window.units(), value, ParaProf.defaultNumberPrecision);
        stringWidth = fmFont.stringWidth(s);
        //Now draw the percent value to the left of the bar.
        stringStart = barXCoord - xLength - stringWidth - 5;
        g2D.drawString(s, stringStart, yCoord);
        g2D.drawString(text, (barXCoord + 5), yCoord);
    }

    public void mouseClicked(MouseEvent evt) {
        try {
            //Get the location of the mouse.
            int xCoord = evt.getX();
            int yCoord = evt.getY();

            //Get the number of times clicked.
            int clickCount = evt.getClickCount();

            PPFunctionProfile ppFunctionProfile = null;

            //Calculate which PPFunctionProfile was clicked on.
            int index = (yCoord) / (ppTrial.getPreferencesWindow().getBarSpacing());

            if (list != null && index < list.size()) {
                ppFunctionProfile = (PPFunctionProfile) list.get(index);

                if ((evt.getModifiers() & InputEvent.BUTTON1_MASK) == 0) {
                    if (xCoord > barXCoord) { //barXCoord should have been set during the last render.
                        ParaProfUtils.handleThreadClick(ppTrial, ppFunctionProfile.getThread(), this, evt);
                    } else {
                        JPopupMenu popup = ParaProfUtils.createFunctionClickPopUp(ppTrial,
                                ppFunctionProfile.getFunction(), this);
                        popup.show(this, evt.getX(), evt.getY());
                    }
                    return;
                }

                if (xCoord > barXCoord) { //barXCoord should have been set during the last render.

                    ThreadDataWindow threadDataWindow = new ThreadDataWindow(ppTrial, ppFunctionProfile.getNodeID(),
                            ppFunctionProfile.getContextID(), ppFunctionProfile.getThreadID());
                    ppTrial.getSystemEvents().addObserver(threadDataWindow);
                    threadDataWindow.show();
                } else {
                    ppTrial.toggleHighlightedFunction(function);
                }
            }
        } catch (Exception e) {
            ParaProfUtils.handleException(e);
        }
    }

    public void mousePressed(MouseEvent evt) {
    }

    public void mouseReleased(MouseEvent evt) {
    }

    public void mouseEntered(MouseEvent evt) {
    }

    public void mouseExited(MouseEvent evt) {
    }

    public Dimension getImageSize(boolean fullScreen, boolean header) {
        Dimension d = null;
        if (fullScreen)
            d = this.getSize();
        else
            d = window.getSize();

        if (header) {
            d.setSize(d.getWidth(), d.getHeight() + lastHeaderEndPosition);
        } else {
            d.setSize(d.getWidth(), d.getHeight());
        }

        return d;
    }

    public void setBarLength(int barLength) {
        this.barLength = Math.max(1, barLength);
        this.repaint();
    }

    //This method sets both xPanelSize and yPanelSize.
    private boolean resizePanel(FontMetrics fmFont, int barXCoord) {
        boolean resized = false;
        int newYPanelSize = ((window.getData().size()) + 1) * barSpacing + 10;
        int[] nct = ppTrial.getMaxNCTNumbers();
        String nctString = "n,c,t " + nct[0] + "," + nct[1] + "," + nct[2];

        int newXPanelSize = barXCoord + 5 + (fmFont.stringWidth(nctString)) + 25;
        if ((newYPanelSize != yPanelSize) || (newXPanelSize != xPanelSize)) {
            yPanelSize = newYPanelSize;
            xPanelSize = newXPanelSize;
            this.setSize(new java.awt.Dimension(xPanelSize, yPanelSize));
            resized = false;
        }
        return resized;
    }

    public Dimension getPreferredSize() {
        return new Dimension(xPanelSize, yPanelSize);
    }

}