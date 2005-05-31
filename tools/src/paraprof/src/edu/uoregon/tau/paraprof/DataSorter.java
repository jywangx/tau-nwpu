package edu.uoregon.tau.paraprof;

import java.util.*;
import edu.uoregon.tau.dms.dss.*;
import edu.uoregon.tau.paraprof.enums.*;

/**
 * DataSorter.java
 * This object manages data for the various windows giving them the capability to show only
 * functions that are in groups supposed to be shown. 
 *  
 * 
 * <P>CVS $Id: DataSorter.java,v 1.10 2005/05/31 23:21:47 amorris Exp $</P>
 * @author	Alan Morris, Robert Bell
 * @version	$Revision: 1.10 $
 */
public class DataSorter {

    private ParaProfTrial trial = null;
    private double maxExclusiveSum = 0;
    private double maxExclusives[];

    private int selectedMetricID;
    private boolean descendingOrder;
    private boolean showAsPercent;
    private SortType sortType = SortType.MEAN_VALUE;
    private ValueType valueType = ValueType.EXCLUSIVE;
    private UserEventValueType userEventValueType = UserEventValueType.NUMSAMPLES;

    public DataSorter(ParaProfTrial trial) {
        this.trial = trial;
        this.selectedMetricID = trial.getDefaultMetricID();
    }

    public UserEventValueType getUserEventValueType() {
        return userEventValueType;
    }

    public void setUserEventValueType(UserEventValueType userEventValueType) {
        this.userEventValueType = userEventValueType;
    }

    public boolean isTimeMetric() {
        String metricName = trial.getMetricName(this.getSelectedMetricID());
        metricName = metricName.toUpperCase();
        if (metricName.indexOf("TIME") == -1)
            return false;
        else
            return true;
    }

    public boolean isDerivedMetric() {

        // We can't do this, HPMToolkit stuff has /'s and -'s all over the place
        //String metricName = this.getMetricName(this.getSelectedMetricID());
        //if (metricName.indexOf("*") != -1 || metricName.indexOf("/") != -1)
        //    return true;
        return trial.getMetric(this.getSelectedMetricID()).getDerivedMetric();
    }

    public void setSelectedMetricID(int metric) {
        this.selectedMetricID = metric;
    }

    public int getSelectedMetricID() {
        return selectedMetricID;
    }

    public void setDescendingOrder(boolean descendingOrder) {
        this.descendingOrder = descendingOrder;
    }

    public boolean getDescendingOrder() {
        return this.descendingOrder;
    }

    public void setShowAsPercent(boolean showAsPercent) {
        this.showAsPercent = showAsPercent;
    }

    public boolean getShowAsPercent() {
        return showAsPercent;
    }

    public void setSortType(SortType sortType) {
        this.sortType = sortType;
    }

    public SortType getSortType() {
        return this.sortType;
    }

    public void setValueType(ValueType valueType) {
        this.valueType = valueType;
    }

    public ValueType getValueType() {
        return this.valueType;
    }

    public List getUserEventProfiles(int nodeID, int contextID, int threadID) {

        UserEventProfile userEventProfile = null;
        List list = ((edu.uoregon.tau.dms.dss.Thread) trial.getDataSource().getThread(nodeID, contextID, threadID)).getUserEventProfiles();

        List newList = new ArrayList();

        for (Iterator e1 = list.iterator(); e1.hasNext();) {
            userEventProfile = (UserEventProfile) e1.next();
            if (userEventProfile != null) {
                PPUserEventProfile ppUserEventProfile = new PPUserEventProfile(this, nodeID, contextID, threadID,
                        userEventProfile);
                newList.add(ppUserEventProfile);
            }
        }
        Collections.sort(newList);
        return newList;
    }

    public List getFunctionProfiles(int nodeID, int contextID, int threadID) {
        List newList = null;

        edu.uoregon.tau.dms.dss.Thread thread;

        if (nodeID == -1) { // mean
            thread = trial.getDataSource().getMeanData();
        } else {
            thread = trial.getDataSource().getThread(nodeID, contextID, threadID);
        }
        List functionList = thread.getFunctionProfiles();
        newList = new ArrayList();

        for (int i = 0; i < functionList.size(); i++) {
            FunctionProfile functionProfile = (FunctionProfile) functionList.get(i);
            if (functionProfile != null) {
                if (trial.displayFunction(functionProfile.getFunction())) {
                    PPFunctionProfile ppFunctionProfile = new PPFunctionProfile(this, thread, functionProfile);
                    newList.add(ppFunctionProfile);
                }
            }
        }
        Collections.sort(newList);
        return newList;
    }

    public List getAllFunctionProfiles() {
        List threads = new ArrayList();

        edu.uoregon.tau.dms.dss.Thread thread = trial.getDataSource().getMeanData();

        PPThread ppThread = new PPThread(thread, this.trial);

        for (Iterator e4 = thread.getFunctionProfiles().iterator(); e4.hasNext();) {
            FunctionProfile functionProfile = (FunctionProfile) e4.next();
            if ((functionProfile != null) && (trial.displayFunction(functionProfile.getFunction()))) {
                PPFunctionProfile ppFunctionProfile = new PPFunctionProfile(this, thread, functionProfile);
                ppThread.addFunction(ppFunctionProfile);
            }
        }
        Collections.sort(ppThread.getFunctionList());
        threads.add(ppThread);

        // reset this in case we are switching metrics
        maxExclusiveSum = 0;

        maxExclusives = new double[trial.getDataSource().getNumFunctions()];

        for (Iterator it = trial.getDataSource().getAllThreads().iterator(); it.hasNext();) {
            thread = (edu.uoregon.tau.dms.dss.Thread) it.next();

            //Counts the number of ppFunctionProfiles that are actually added.
            //It is possible (because of selection criteria - groups for example) to filter
            //out all functions on a particular thread. The default at present is not to add.

            int counter = 0; //Counts the number of PPFunctionProfile that are actually added.
            ppThread = new PPThread(thread, this.trial);

            double sum = 0.0;

            //Do not add thread to the context until we have verified counter is not zero (done after next loop).
            //Now enter the thread data loops for this thread.
            for (Iterator e4 = thread.getFunctionProfiles().iterator(); e4.hasNext();) {
                FunctionProfile functionProfile = (FunctionProfile) e4.next();
                if ((functionProfile != null) && (trial.displayFunction(functionProfile.getFunction()))) {
                    PPFunctionProfile ppFunctionProfile = new PPFunctionProfile(this, thread, functionProfile);
                    ppThread.addFunction(ppFunctionProfile);
                    counter++;

                    sum += ppFunctionProfile.getExclusiveValue();

                    maxExclusives[functionProfile.getFunction().getID()] = Math.max(
                            maxExclusives[functionProfile.getFunction().getID()], ppFunctionProfile.getExclusiveValue());
                }
            }

            if (sum > maxExclusiveSum) {
                maxExclusiveSum = sum;
            }

            //Sort thread and add to context if required (see above for an explanation).
            if (counter != 0) {
                Collections.sort(ppThread.getFunctionList());
                threads.add(ppThread);
            }
        }
        return threads;
    }

    public List getFunctionData(Function function, boolean includeMean) {
        List newList = new ArrayList();

        edu.uoregon.tau.dms.dss.Thread thread;

        if (includeMean) {
            thread = trial.getDataSource().getMeanData();
            FunctionProfile functionProfile = thread.getFunctionProfile(function);
            if (functionProfile != null) {
                //Create a new thread data object.
                PPFunctionProfile ppFunctionProfile = new PPFunctionProfile(this, thread, functionProfile);
                newList.add(ppFunctionProfile);
            }
        }

        for (Iterator it = trial.getDataSource().getAllThreads().iterator(); it.hasNext();) {
            thread = (edu.uoregon.tau.dms.dss.Thread) it.next();
            FunctionProfile functionProfile = thread.getFunctionProfile(function);
            if (functionProfile != null) {
                //Create a new thread data object.
                PPFunctionProfile ppFunctionProfile = new PPFunctionProfile(this, thread, functionProfile);
                newList.add(ppFunctionProfile);
            }
        }
        Collections.sort(newList);
        return newList;
    }

    public List getUserEventData(UserEvent userEvent) {
        List newList = new ArrayList();

        UserEventProfile userEventProfile;

        PPUserEventProfile ppUserEventProfile;

        for (Iterator it = trial.getDataSource().getAllThreads().iterator(); it.hasNext();) {
            edu.uoregon.tau.dms.dss.Thread thread = (edu.uoregon.tau.dms.dss.Thread) it.next();

            userEventProfile = thread.getUserEventProfile(userEvent);
            if (userEventProfile != null) {
                //Create a new thread data object.
                ppUserEventProfile = new PPUserEventProfile(this, thread.getNodeID(), thread.getContextID(),
                        thread.getThreadID(), userEventProfile);
                newList.add(ppUserEventProfile);
            }
        }
        Collections.sort(newList);
        return newList;
    }

    // returns the maximum exclusive sum over all threads
    public double getMaxExclusiveSum() {
        return maxExclusiveSum;
    }

    public double[] getMaxExclusives() {
        return maxExclusives;
    }

}