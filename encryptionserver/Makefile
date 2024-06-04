CXX=g++
HXXPATH=./
COMMONINCPATH=../common/inc/
COMMONSRCPATH=../common/src/
SSLINCPATH=../common/openssl-0.9.7e/include/
SSLPATH=../common/openssl-0.9.7e/
all:	encryptionserver

encryptionserver:	encryptionserver.o ptl.o tcpsock.o configclass.o debugclass.o common.o
			$(CXX) -o  encryptionserver encryptionserver.o ptl.o  tcpsock.o configclass.o debugclass.o common.o ../api/lib/api_mt.a  $(SSLPATH)/libssl.a $(SSLPATH)/libcrypto.a -pthread

debugclass.o:	$(COMMONSRCPATH)debugclass.cpp
		$(CXX) -c -g $(COMMONSRCPATH)debugclass.cpp -I$(COMMONINCPATH) -D_REENTRANT

configclass.o:	$(COMMONSRCPATH)configclass.cpp 
		$(CXX) -c -g $(COMMONSRCPATH)configclass.cpp -I$(COMMONINCPATH) -D_REENTRANT

encryptionserver.o:	encryptionserver.cxx encryptionserver.hxx ptl.hxx tcpsock.hxx common.hxx
			$(CXX)  -c -g encryptionserver.cxx -I$(COMMONINCPATH) -I$(SSLINCPATH) -I$(HXXPATH) -I../api/inc -D_REENTRANT -pthread

common.o:	common.cxx common.hxx
		$(CXX) -c -g common.cxx -D_REENTRANT

tcpsock.o:	tcpsock.cxx tcpsock.hxx
		$(CXX) -c -g tcpsock.cxx -I$(HXXPATH) -D_REENTRANT

ptl.o:		ptl.cxx ptl.hxx
		$(CXX) -c -g ptl.cxx -I$(HXXPATH) -D_REENTRANT

