# This Jobdatabas-Class interacts with the mps.db file.
# It's member-variables are often called in the mps_... scripts.
#
# Meaning of the database variables: (still need to work on these)
#
# (1) Header
#       header          - version information
#       batchScript     - base script for serial job
#       cfgTemplate     - template for cfg file
#       infiList        - list of input files to be serialized
#       classInf        - batch class information (might contain two ':'-separated)
#       addFiles        - job name for submission
#       driver          - specifies whether merge job is foreseen
#       nJobs           - number of serial jobs (not including merge job)
#       mergeScript     - base script for merge job
#       mssDir          - directory for mass storage (e.g. Castor)
#       updateTime      - time of last update (seconds since 1970)
#       updateTimeHuman - time of last update (human readable)
#       elapsedTime     - seconds since last update
#       mssDirPool      - pool for $mssDir (e.g. cmscaf/cmscafuser)
#       pedeMem         - Memory allocated for pede
#       spare1
#       spare2
#       spare3

# (2) Job-level variables/lists
#       JOBNUMBER   - ADDED, selfexplanatory
#       JOBDIR      - name of job directory (not full path)
#       JOBSTATUS   - status of job
#       JOBRUNTIME  - present CPU time of job
#       JOBNEVT     - number of events processed by job
#       JOBHOST     - presently used to store remark
#       JOBINCR     - CPU increment since last check
#       JOBREMARK   - comment
#       JOBSP1      - spare
#       JOBSP2      - possible weight for pede
#       JOBSP3      - possible name as given to mps_setup.pl -N <name> ...
#       JOBID       - ID of the LSF/HTCondor job

from builtins import range
import datetime
import time
import os
import sys
import re
import math
import fileinput

#-------------------------------------------------------------------------------
class jobdatabase:

    JOBNUMBER, JOBDIR, JOBID, JOBSTATUS, JOBNTRY, JOBRUNTIME, JOBNEVT, JOBHOST, JOBINCR, \
    JOBREMARK, JOBSP1, JOBSP2, JOBSP3 = ([] for i in range(13))

    header, batchScript, cfgTemplate, infiList, classInf, addFiles, driver, mergeScript, \
    mssDir, updateTimeHuman, mssDirPool, spare1, spare2, spare3 = ('' for i in range(14))

    updateTime, elapsedTime, pedeMem , nJobs = -1, -1, -1, -1

    #-------------------------------------------------------------------------------
    # parses the mps.db file into the member variables and arrays
    __default_db_file_name = "mps.db"
    def read_db(self, db_file_name = __default_db_file_name):
        try:
            DBFILE = open(db_file_name,'r')
        except IOError as e:
            if e.args != (2, 'No such file or directory'):
                raise
            else:
                if db_file_name == jobdatabase.__default_db_file_name:
                    msg = ("No 'mps.db' found. Make sure you are in a campaign "
                           "directory and that the campaign is set up.")
                else:
                    msg = "Database file '"+db_file_name+"' not found. Exiting."
                print(msg)
                sys.exit(1)

        #read infolines at the top, used rstrip to delete the '\n'
        self.header          = DBFILE.readline().strip()
        self.batchScript     = DBFILE.readline().rstrip('\n')
        self.cfgTemplate     = DBFILE.readline().rstrip('\n')
        self.infiList        = DBFILE.readline().rstrip('\n')
        self.classInf        = DBFILE.readline().rstrip('\n')   #formerly named 'class' ->conflict
        self.addFiles        = DBFILE.readline().rstrip('\n')
        self.driver          = DBFILE.readline().rstrip('\n')
        self.mergeScript     = DBFILE.readline().rstrip('\n')
        self.mssDir          = DBFILE.readline().rstrip('\n')
        self.updateTime      = int(DBFILE.readline())
        self.updateTimeHuman = DBFILE.readline().rstrip('\n')
        self.elapsedTime     = int(DBFILE.readline())
        self.mssDirPool      = DBFILE.readline().rstrip('\n')
        self.pedeMem         = int(DBFILE.readline())
        self.spare1          = DBFILE.readline().rstrip('\n')
        self.spare2          = DBFILE.readline().rstrip('\n')
        self.spare3          = DBFILE.readline().rstrip('\n')

        #read actual jobinfo into arrays
        self.nJobs = 0
        milleJobs = 0


        for line in DBFILE:
            if line.strip() == "": continue # ignore empty lines
            line = line.rstrip('\n')        # removes the pesky \n from line
            parts = line.split(":")         # read each line and split into parts list
            self.JOBNUMBER.append(int(parts[0]))
            self.JOBDIR.append(parts[1].strip())
            self.JOBID.append(parts[2])
            self.JOBSTATUS.append(parts[3].strip())
            self.JOBNTRY.append(int(parts[4]))
            self.JOBRUNTIME.append(int(parts[5]))   #int float?
            self.JOBNEVT.append(int(parts[6]))
            self.JOBHOST.append(parts[7].strip())
            self.JOBINCR.append(int(parts[8]))
            self.JOBREMARK.append(parts[9].strip())
            self.JOBSP1.append(parts[10].strip())
            self.JOBSP2.append(parts[11].strip())
            self.JOBSP3.append(parts[12].strip())

            #count number of jobs
            if not self.JOBDIR[self.nJobs].startswith("jobm"):
                milleJobs += 1
            self.nJobs += 1
        self.nJobs = milleJobs

        DBFILE.close()



    #-------------------------------------------------------------------------------
    # prints the member varaiables and arrays to the terminal
    def print_memdb(self):
        #print metainfo
        print("\n=== mps database printout ===\n")
        print(self.header)
        print('Script:\t\t',    self.batchScript)
        print('cfg:\t\t',       self.cfgTemplate)
        print('files:\t\t',     self.infiList)
        print('class:\t\t',     self.classInf)
        print('name:\t\t',      self.addFiles)
        print('driver:\t\t',    self.driver)
        print('mergeScript:\t', self.mergeScript)
        print('mssDir:\t\t',    self.mssDir)
        print('updateTime:\t',  self.updateTimeHuman)
        print('elapsed:\t',     self.elapsedTime)
        print('mssDirPool:\t',  self.mssDirPool)
        print('pedeMem:\t',             self.pedeMem, '\n')

        #print interesting Job-level lists ---- to add: t/evt, fix remarks
        headFmt = '###     dir      jobid    stat  try  rtime      nevt  remark   weight  name'
        jobFmt = '%03d  %6s  %10s%6s  %3d  %5d  %6d  %12s  %5s  %s'
        mrgFmt = '%s  %6s  %10s%6s  %3d  %5d  %6d  %12s  %5s  %s'
        if self.nJobs>999:
            headFmt = '####     dir       jobid  stat  try  rtime      nevt    remark   weight  name'
            jobFmt = '%04d  %7s  %10s%6s  %3d  %5d  %6d  %12s  %5s  %s'
            mrgFmt = '%s   %7s  %10s%6s  %3d  %5d  %6d  %12s  %5s  %s'
        if self.nJobs>9999:
            jobFmt = '%d  %s  %10s%6s  %3d  %5d  %6d  %12s  %5s  %s'
            mrgFmt = '%s    %8s  %10s%6s  %3d  %5d  %6d  %12s  %5s  %s'
        print(headFmt)
        print("------------------------------------------------------------------------------")
        for i in range(self.nJobs):
            remarkField = self.JOBHOST[i]
            if self.JOBSTATUS[i] == "FAIL":
                remarkField = self.JOBREMARK[i]
            print(jobFmt % (
                self.JOBNUMBER[i],
                self.JOBDIR[i],
                self.JOBID[i],
                self.JOBSTATUS[i][:6],
                self.JOBNTRY[i],
                self.JOBRUNTIME[i],
                self.JOBNEVT[i],
                remarkField[:12],
                self.JOBSP2[i],
                self.JOBSP3[i]))

        #print merge Jobs if merge mode
        if self.driver == 'merge':
            for i in range(self.nJobs,len(self.JOBDIR)):
                remarkField = self.JOBHOST[i]
                if (self.JOBSTATUS[i] == "FAIL") or (self.JOBSTATUS[i] == "WARN"):
                    remarkField = self.JOBREMARK[i]
                print(mrgFmt % (
                    'MMM',
                    self.JOBDIR[i],
                    self.JOBID[i],
                    self.JOBSTATUS[i][:6],
                    self.JOBNTRY[i],
                    self.JOBRUNTIME[i],
                    self.JOBNEVT[i],
                    remarkField[:12],
                    self.JOBSP2[i],
                    self.JOBSP3[i]))

        #print summed info
        totalEvents = sum(self.JOBNEVT[:self.nJobs])
        totalCpu    = sum(self.JOBRUNTIME[:self.nJobs])
        meanCpuPerEvent = 0.
        if totalEvents > 0:
            meanCpuPerEvent = float(totalCpu)/totalEvents
        print("------------------------------------------------------------------------------")
        print("\t\t\t\t\tEvent total:\t",       totalEvents)
        print("\t\t\t\t\tCPU total:\t",         totalCpu,               's')
        print("\t\t\t\t\tMean CPU/event:\t",meanCpuPerEvent,'s')





    #-------------------------------------------------------------------------------
    # writes a new mps.db file from the members. Replaces the old mps.db
    def write_db(self):
        self.header = "mps database schema 4.0"
        self.currentTime = int(time.time())
        self.elapsedTime = 0;
        if self.updateTime != 0:
            self.elapsedTime = self.currentTime - self.updateTime
        self.updateTime = self.currentTime
        self.updateTimeHuman = str(datetime.datetime.today())   #no timezone :(
        self.spare1 = "-- unused --"
        self.spare2 = "-- unused --"
        self.spare3 = "-- unused --"

        #if mps.db already exists, backup as mps.db~ (in case of interupt during write)
        os.system('[[ -a mps.db ]] && cp -p mps.db mps.db~')

        #write mps.db header
        DBFILE = open ("mps.db", "w")
        headData = [ self.header, self.batchScript, self.cfgTemplate, self.infiList,
                     self.classInf, self.addFiles, self.driver, self.mergeScript,
                     self.mssDir, self.updateTime, self.updateTimeHuman,
                     self.elapsedTime, self.mssDirPool, self.pedeMem,
                     self.spare1, self.spare2, self.spare3 ]
        for item in headData:
            DBFILE.write("%s\n" % item)

        #write mps.db jobinfo
        for i in range(len(self.JOBID)):
            DBFILE.write('%d:%s:%s:%s:%s:%s:%s:%s:%s:%s:%s:%s:%s\n' %
                         (i+1,
                          self.JOBDIR[i],
                          self.JOBID[i],
                          self.JOBSTATUS[i],
                          self.JOBNTRY[i],
                          self.JOBRUNTIME[i],
                          self.JOBNEVT[i],
                          self.JOBHOST[i],
                          self.JOBINCR[i],
                          self.JOBREMARK[i],
                          self.JOBSP1[i],
                          self.JOBSP2[i],
                          self.JOBSP3[i]))
        DBFILE.close()

    #-------------------------------------------------------------------------------
    # returns job class as stored in db
    # one and only argument may be "mille" or "pede" for mille or pede jobs
    def get_class(self, argument=''):
        CLASSES = self.classInf.split(':')
        if len(CLASSES)<1 or len(CLASSES)>2:
            print('\nget_class():\n  class must be of the form \'class\' or \'classMille:classPede\', but is \'%s\'!\n\n', classInf)
            sys.exit(1)
        elif argument == 'mille':
            return CLASSES[0]
        elif argument == 'pede':
            if len(CLASSES) == 1:
                return CLASSES[0]
            elif len(CLASSES) == 2:
                return CLASSES[1]
        else:
            print('\nget_class():\n  Know class only for \'mille\' or \'pede\', not %s!\n\n' %argument)
            sys.exit(1)

    #-------------------------------------------------------------------------------
    #  Take card file, blank all INFI directives and insert the INFI directives 
    #  from the modifier file instead
    def mps_splice(self,inCfg='',modCfg='',outCfg='the.py',isn=0,skip_events=0,max_events=0):

        with open(inCfg, 'r') as INFILE:
            body = INFILE.read()

        # read modifier file
        with open(modCfg, 'r') as MODFILE:
            mods = MODFILE.read()
        mods = mods.strip()

        # prepare the new fileNames directive. Delete first line if necessary.
        fileNames = mods.split('\n')
        if 'CastorPool=' in fileNames[0]:
            del fileNames[0]

        # replace ISN number (input is a string of three digit number with leading zeros though)
        body = re.sub(re.compile('ISN',re.M), isn, body)

        # print to outCfg
        with open(outCfg, 'w') as OUTFILE:
            OUTFILE.write(body)

        # Number of total files and number of extends for cms.vstring are needed
        numberOfFiles   = len(fileNames)
        numberOfExtends = int(math.ceil(numberOfFiles/255.))

        # Create and insert the readFile.extend lines
        for j in range(numberOfExtends):
            insertBlock = "readFiles.extend([\n    "
            i=0
            currentStart = j*255
            while (i<255) and ((currentStart+i)<numberOfFiles):
                entry = fileNames[currentStart+i].strip()
                if (i==254) or ((currentStart+i+1)==numberOfFiles):
                    insertBlock += "\'"+entry+"\'])\n"
                else:
                    insertBlock += "\'"+entry+"\',\n    "
                i+=1

            for line in fileinput.input(outCfg, inplace=1):
                print(line,end='')
                if re.match('readFiles\s*=\s*cms.untracked.vstring()',line):
                    print(insertBlock,end='')

        if skip_events != 0:
            with open(outCfg, "a") as f:
                f.write("process.source.skipEvents = cms.untracked.uint32({0:d})\n"
                        .format(skip_events))

        if max_events != 0:
            with open(outCfg, "a") as f:
                f.write("process.maxEvents = cms.untracked.PSet(input = "
                        "cms.untracked.int32({0:d}))\n".format(max_events))

