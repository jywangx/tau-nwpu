#!/usr/bin/env python

"""A python script to convert TAU profiles to JSON
"""

from __future__ import print_function
import os
import sys
import argparse
import json
from collections import OrderedDict

workflow_metadata_str = "Workflow metadata"
metadata_str = "metadata"
global_data = None

"""
    Parse the "invalid" TAU XML
"""
def parse_tau_xml(instring):
    mydict = OrderedDict()
    tmp = instring
    metadata_tokens = tmp.split("<metadata>",1)
    metadata_tokens = metadata_tokens[1].split("</metadata>",1)
    tmp = metadata_tokens[0]
    while len(tmp) > 0:
        attribute_tokens = tmp.split("<attribute>",1)
        attribute_tokens = attribute_tokens[1].split("</attribute>",1)
        attribute = attribute_tokens[0]
        tmp = attribute_tokens[1]
        name_tokens = attribute.split("<name>",1)
        name_tokens = name_tokens[1].split("</name>",1)
        name = name_tokens[0]
        value_tokens = name_tokens[1].split("<value>",1)
        value_tokens = value_tokens[1].split("</value>",1)
        value = value_tokens[0]
        # print(name,value)
        if name == "Metric Name":
            continue
        mydict[name] = value
        if tmp == "</metadata>":
            break
    return mydict       

"""
This method will parse the arguments.
"""
def parse_args(arguments):
    global workflow_metadata_str
    global metadata_str
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('indir', help="One or more input directories (can be raw TAU profiles or generated from tau_coalesce)", nargs='+')
    parser.add_argument('-w', '--workflow', help="Workflow Metadata file (JSON)",
                        default=None)
    parser.add_argument('-o', '--outfile', help="Output file instead of print to stdout (optional)",
                        default=sys.stdout, type=argparse.FileType('w'))

    args = parser.parse_args(arguments)
    # print(args)
    return args

def extract_workflow_metadata(component_name, thread_name, max_inclusive):
    global global_data
    # This is the root rank/thread for an application, so extract some key info
    workflow_dict = global_data[workflow_metadata_str]
    app_dict = global_data[component_name][metadata_str][thread_name]
    time_stamp = app_dict["Timestamp"]
    local_time = app_dict["Local Time"]
    if time_stamp != None:
        for wc in workflow_dict["Workflow Component"]:
            if wc["name"] == component_name:
                wc["start-timestamp"] = time_stamp
                wc["end-timestamp"] = str(long(time_stamp) + max_inclusive)
                wc["Local-Time"] = local_time
        for wc in workflow_dict["Workflow Instance"]:
            if "timestamp" not in wc or wc["timestamp"] > time_stamp:
                wc["timestamp"] = time_stamp
                wc["Local-Time"] = local_time
    app_name = app_dict["Executable"]
    found = False
    app_id = 0
    for app in workflow_dict["Application"]:
        if app["name"] in component_name or app["name"] == app_name:
            Found = True
            app_id = app["id"]
            break
    if not found:
        app = OrderedDict()
        app_id = len(workflow_dict["Application"]) + 1
        app["id"] = app_id
        app["location-id"] = 1
        app["name"] = app_name
        app["version"] = "0.1"
        app["uri"] = ""
    	workflow_dict["Application"].append(app)
    app_instance = OrderedDict()
    app_instance["id"] = len(workflow_dict["Application-instance"]) + 1
    app_instance["process-id"] = app_dict["pid"]
    app_instance["application-id"] = app_id
    app_instance["event-type"] = ""
    app_instance["start-timestamp"] = time_stamp
    app_instance["end-timestamp"] = str(long(time_stamp) + max_inclusive)
    app_instance["Local-Time"] = local_time
    workflow_dict["Application-instance"].append(app_instance)

def parse_functions(node, context, thread, infile, data, num_functions, function_map):
    max_inclusive = 0
    # Function looks like:
    # "".TAU application" 1 0 8626018 8626018 0 GROUP="TAU_USER""
    if num_functions > 0:
        """
        if "Timers" not in data:
            data["Timers"] = OrderedDict()
        if "Timer Measurement Columns" not in data:
            data["Timer Measurement Columns"] = list()
            data["Timer Measurement Columns"].append("Process Index")
            #data["Timer Measurement Columns"].append("Context Index")
            data["Timer Measurement Columns"].append("Thread Index")
            data["Timer Measurement Columns"].append("Function Index")
            data["Timer Measurement Columns"].append("Calls")
            data["Timer Measurement Columns"].append("Subroutines")
            data["Timer Measurement Columns"].append("Exclusive TIME")
            data["Timer Measurement Columns"].append("Inclusive TIME")
        """
        if "Timers" not in data:
            data["Timers"] = list()
        for i in range(0,num_functions):
            line = infile.readline()
            tokens = line.split("\"",2)
            function_name = tokens[1].strip()
            stats = tokens[2]
            """
            if function_name not in function_map:
                #data["Timers"][function_name] = len(data["Timers"])
                function_map[function_name] = len(data["Timers"])
                data["Timers"][len(data["Timers"])] = function_name
            """
            # split the stats
            tokens = stats.strip().split(" ", 6)
            """
            timer = list()
            timer.append(int(node))
            #timer.append(int(context))
            timer.append(int(thread))
            # function index
            timer.append(function_map[function_name])
            # calls
            timer.append(long(tokens[0]))
            # Subroutines
            timer.append(long(tokens[1]))
            # Exclusive
            timer.append(long(tokens[2]))
            # Inclusive
            timer.append(long(tokens[3]))
            # Profile Calls
            #timer.append(long(tokens[4]))
            data["Timer Measurements"].append(timer)
            """
            timer = OrderedDict()
            timer["process index"] = int(node)
            timer["thread index"] = int(thread)
            timer["Function"] = function_name
            timer["Calls"] = long(tokens[0])
            timer["Subroutines"] = long(tokens[1])
            timer["Exclusive Time"] = long(tokens[2])
            timer["Inclusive Time"] = long(tokens[3])
            data["Timers"].append(timer)
            if max_inclusive < long(tokens[3]):
                max_inclusive = long(tokens[3])
    return max_inclusive
    
def parse_aggregates(node, context, thread, infile, data):
    aggregates = infile.readline()
    tokens = aggregates.split()
    num_aggregates = int(tokens[0])
    # data["Num Aggregates"] = tokens[0]
    for i in range(1,num_aggregates):
        # do nothing
        line = infile.readline()

def parse_counters(node, context, thread, infile, data, counter_map):
    userevents = infile.readline()
    if (userevents == None or userevents == ""):
        return
    tokens = userevents.split()
    num_userevents = int(tokens[0])
    # data["Num Counters"] = tokens[0]
    # ignore the header line
    line = infile.readline()
    for i in range(1,num_userevents):
        """
        if "Counters" not in data:
            data["Counters"] = OrderedDict()
        if "Counter Measurement Columns" not in data:
            data["Counter Measurement Columns"] = list()
            data["Counter Measurement Columns"].append("Process Index")
            #data["Counter Measurement Columns"].append("Context Index")
            data["Counter Measurement Columns"].append("Thread Index")
            data["Counter Measurement Columns"].append("Function Index")
            data["Counter Measurement Columns"].append("Sample Count")
            data["Counter Measurement Columns"].append("Maximum")
            data["Counter Measurement Columns"].append("Minimum")
            data["Counter Measurement Columns"].append("Mean")
            data["Counter Measurement Columns"].append("SumSqr")
        """
        if "Counters" not in data:
            data["Counters"] = list()
        for i in range(0,num_userevents):
            line = infile.readline()
            line = line.strip()
            if len(line) == 0:
                break
            tokens = line.split("\"",2)
            counter_name = tokens[1].strip()
            stats = tokens[2]
            if counter_name == "(null)":
                continue
            """
            if counter_name not in counter_map:
                #data["Counters"][counter_name] = len(data["Counters"])
                counter_map[counter_name] = len(data["Counters"])
                data["Counters"][len(data["Counters"])] = counter_name
            """
            # split the stats
            tokens = stats.strip().split(" ", 5)
            """
            counter = list()
            counter.append(int(node))
            counter.append(int(context))
            counter.append(int(thread))
            # function index
            counter.append(counter_map[counter_name])
            # numevents
            counter.append(float(tokens[0]))
            # max
            counter.append(float(tokens[1]))
            # min
            counter.append(float(tokens[2]))
            # mean
            counter.append(float(tokens[3]))
            # sumsqr
            counter.append(float(tokens[4]))
            data["Counter Measurements"].append(counter)
            """
            counter = OrderedDict()
            counter["process index"] = int(node)
            counter["thread index"] = int(thread)
            counter["Counter"] = counter_name
            counter["Num Events"] = long(tokens[0])
            counter["Max Value"] = float(tokens[1])
            counter["Min Value"] = float(tokens[2])
            counter["Mean Value"] = float(tokens[3])
            counter["SumSqr Value"] = float(tokens[4])
            data["Counters"].append(counter)

def parse_profile(indir, profile, application_metadata, function_map, counter_map):
    global metadata_str
    global global_data
    # FIrst, split the name of the profile to get the node, context, thread
    tokens = profile.split(".")
    node = tokens[1]
    context = tokens[2]
    thread = tokens[3]
    # Open the file, get the first line
    infile = open(os.path.join(indir, profile), "r")
    functions = infile.readline()
    # First line looks like:
    # "16 templated_functions_MULTI_TIME"
    tokens = functions.split()
    num_functions = int(tokens[0])
    # application_metadata["Metric"] = tokens[1]
    # The header for the functions looks like this:
    # "# Name Calls Subrs Excl Incl ProfileCalls # <metadata>...</metadata>"
    header = infile.readline()
    tokens = header.split("#",2)
    # Parse the metadata
    thread_name = "Rank " + str(node) + ", Thread " + str(thread)
    application_metadata[metadata_str][thread_name] = parse_tau_xml(str.strip(tokens[2]))
    # Parse the functions
    max_inclusive = parse_functions(node, context, thread, infile, application_metadata, num_functions, function_map)
    if node == "0" and thread == "0":
        extract_workflow_metadata(indir, thread_name, max_inclusive)
    # Parse the aggregates
    parse_aggregates(node, context, thread, infile, application_metadata)
    # Parse the counters
    parse_counters(node, context, thread, infile, application_metadata, counter_map)

"""
This method will parse a directory of TAU profiles
"""
def parse_directory(indir, index):
    global metadata_str
    global global_data
    # assume just 1 metric for now...

    # create a dictionary for this application
    application_metadata = OrderedDict()
    #application["source directory"] = indir

    # add the application to the master dictionary
    # tmp = "Application " + str(index)
    #data[tmp] = application
    global_data[indir] = application_metadata
    
    # get the list of profile files
    profiles = [f for f in os.listdir(indir) if (os.path.isfile(os.path.join (indir, f)) and f.startswith("profile."))]
    #application["num profiles"] = len(profiles)

    # sort the profiles alphanumerically
    profiles.sort()

    application_metadata[metadata_str] = OrderedDict()
    function_map = {}
    counter_map = {}
    for p in profiles:
        parse_profile(indir, p, application_metadata, function_map, counter_map)

"""
Main method
"""
def main(arguments):
    global workflow_metadata_str
    global global_data
    # parse the arguments
    args = parse_args(arguments)
    global_data = OrderedDict()
    if args.workflow != None:
        global_data[workflow_metadata_str] = json.load(open(args.workflow), object_pairs_hook=OrderedDict)
    else:
        global_data[workflow_metadata_str] = OrderedDict()
    global_data[workflow_metadata_str]["Application"] = []
    global_data[workflow_metadata_str]["Application-instance"] = []

    index = 1
    for indir in args.indir:
        print ("Processing:", indir)
        parse_directory(indir, index)
        index = index + 1

    # write the JSON output
    if args.outfile == None:
        # json.dump(global_data, args.outfile)
        json.dumps(global_data, indent=2)
    else:
        # json.dump(global_data, args.outfile)
        args.outfile.write(json.dumps(global_data, indent=2))

"""
Call the main function if not called from another python file
"""
if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
