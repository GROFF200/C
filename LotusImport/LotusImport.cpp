#include <lncppapi.h>
#include <iostream.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <fstream.h>

const int LN_ERROR_MESSAGE_LENGTH=2000;
int numLines=0;
struct AddrStruct {
	char *AddrName;
	char *AddrPIN;
};
struct AddrStruct *ImportInfo=new AddrStruct[5000];


int ReadSIPSFile(char *fname) {

	FILE *SIPSFile;
	char *tmp=new char[5000];
	char *tmpstr=new char[5000];
	int  element=0, index=0, counter=0, bigIndex=0;

	SIPSFile=fopen(fname, "rb");
	if (SIPSFile==0) return -1;
	while (fgets(tmp, 5000, SIPSFile)!=NULL) {
		for (int x=0; x<strlen(tmp); x++)
			if (tmp[x]=='\r' || tmp[x]=='\n') tmp[x]='\0';
		strcat(tmp, ";");
		index=0;
		element=0;
		for (x=0; x<strlen(tmp); x++) {
			if (tmp[x]==';') {
				counter=0;
				strcpy(tmpstr, "");
			    for (int y=index; y<x; y++) {
				     tmpstr[counter]=tmp[y];
					 counter++;
				}
				tmpstr[counter]='\0';
				index=y+1;
				if (element==0) {
					ImportInfo[bigIndex].AddrName=new char[strlen(tmpstr)];
					strcpy(ImportInfo[bigIndex].AddrName, tmpstr);
				}
				if (element==1) {
					ImportInfo[bigIndex].AddrPIN=new char[strlen(tmpstr)];
					strcpy(ImportInfo[bigIndex].AddrPIN, tmpstr);
				}
				element++;
			}
		}
		bigIndex++;
	}
	fclose(SIPSFile);
	numLines=bigIndex;
	return 0;
}

//////////////////////////////////
//comparison method that is
//used by qsort()
//////////////////////////////////
int CompareEntries(const void *el1, const void *el2) {

	const struct AddrStruct *tmp1=(AddrStruct *)el1;
	const struct AddrStruct *tmp2=(AddrStruct *)el2;
	return strcmp (tmp1->AddrName, tmp2->AddrName);
}

void OutputDate() {

	time_t longtime;

	time(&longtime);
	printf("TIME: %s\n", ctime(&longtime));
}

void main(int argc, char *argv[]) {

	LNNotesSession  session;
	LNDatabase		db;
	LNDocument		doc;
	LNText			TestEl;
	LNViewFolder	view;

	try {
		session.Init();
		session.GetDatabase("sipsnsf\\sips.nsf", &db, "skytel2/ATC_Cert");
		db.Open();
		OutputDate();
		//Get the view, and delete all documents in the view
		db.GetViewFolder("Corporate Book All", &view);
		view.Open();
		view.DeleteAllEntries();
		view.Close();
		//Read the file and parse it
		ReadSIPSFile("c:\\temp\\test.txt");
		//Sort the contents extracted from the file
		qsort(ImportInfo, numLines, sizeof(ImportInfo[0]), CompareEntries);
		//Import file contents into the database
		for (int x=0; x<numLines; x++) {
		     db.CreateDocument(&doc, "Corporate Address");
		     doc.CreateItem("Name", &TestEl);
		     TestEl << ImportInfo[x].AddrName;
			 doc.CreateItem("Pin", &TestEl);
			 TestEl << ImportInfo[x].AddrPIN;
			 doc.ComputeWithForm(FALSE);
		     doc.Save();
		     doc.Close();
		}
		db.Close();
		OutputDate();
	} catch(LNSTATUS error) {
		char errorBuf[LN_ERROR_MESSAGE_LENGTH];
		LNGetErrorMessage(error, errorBuf);
		cout << "Error:  " << errorBuf << endl;
	}
}

