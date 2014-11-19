/*
 * xemfsruncommandservice.cpp
 *
 *  Created on: 7 nov. 2009
 *      Author: francois
 */

#include <Xemeiah/xemfs/xemfsruncommandservice.h>
#include <Xemeiah/xemfs/xemfsmodule.h>
#include <Xemeiah/xemprocessor/xemservicemodule.h>
#include <Xemeiah/dom/childiterator.h>
#include <Xemeiah/auto-inline.hpp>

#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/timeb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>

#include <errno.h>

#define Log_CmdInfo Debug //< Log_CmdInfo for CommandInfo

namespace Xem
{
  void
  XemFSModule::instructionRunCommandService(__XProcHandlerArgs__)
  {
    XemService* service = new XemFSRunCommandService(getXProcessor(), *this, item);
    service->registerMyself(getXProcessor());
  }

  XemFSRunCommandService&
  XemFSModule::getXemFSRunCommandService(const String& serviceName)
  {
    Service* genericService = getServiceManager().getService(serviceName);
    if (!genericService)
      {
        throwException ( Exception, "Could not fetch service '%s'\n", serviceName.c_str() );
      }

    XemFSRunCommandService* service = dynamic_cast<XemFSRunCommandService*> (genericService);
    if (!service)
      {
        throwException ( Exception, "Service is not a xem-fs:run-command-service !\n" );
      }
    return *service;
  }

  void
  XemFSModule::instructionSendCommand(__XProcHandlerArgs__)
  {
    XemServiceModule& xemServiceModule = XemServiceModule::getMe(
        getXProcessor());
    String serviceName = item.getEvaledAttr(getXProcessor(),
        xemServiceModule.xem_service.name());
    XemFSRunCommandService& service = getXemFSRunCommandService(serviceName);

    String command = item.getEvaledAttr(getXProcessor(), xem_fs.command());

    command += "\n";

    service.sendCommand(command);
  }

  void
  XemFSModule::functionRecvCommand(__XProcFunctionArgs__)
  {
    throwException ( Exception, "This is deprecated !\n" );

    __ui64 minWaitTime = 50;

    if (args.size() < 1 || args.size() > 2 )
      {
        throwException ( Exception, "Wrong number of arguments for xem-fs:recv-command('serviceName')");
      }
    String serviceName = args[0]->toString();

    String waitTimeStr;
    if (args.size() == 2)
      waitTimeStr = args[1]->toString();

    __ui64 waitTime = waitTimeStr.toUI64();

    if (waitTime < minWaitTime)
      waitTime = minWaitTime;

    XemFSRunCommandService& service = getXemFSRunCommandService(serviceName);

    String res = service.recvCommand(waitTime,waitTime*2);
    result.setSingleton(res);
  }

  void XemFSModule::functionGetRecvDocument(__XProcFunctionArgs__)
  {
    if ( args.size() != 1 )
      {
        throwException ( Exception, "Invalid number of arguments for xem-fs:get-recv-document('service-name')\n");
      }

    String serviceName = args[0]->toString();
    XemFSRunCommandService& service = getXemFSRunCommandService(serviceName);

    Document& doc = service.getRecvDocument();
    ElementRef root = doc.getRootElement();
    result.pushBack(root);
  }


  XemFSRunCommandService::XemFSRunCommandService(XProcessor& xproc, XemFSModule& _xemFSModule,
      ElementRef& _configurationElement) :
    XemService(xproc, _configurationElement),
    xemFSModule(_xemFSModule),
    xem_fs(xemFSModule.xem_fs)
  {
    recvDocument = NULL;
    stdinfd = -1;
    stdoutfd = -1;
    stderrfd = -1;

    if ( configurationElement.hasAttr(xem_fs.connect_host()))
      {
        String res = configurationElement.getEvaledAttr(xproc,xem_fs.connect_host());
        host = stringFromAllocedStr(strdup(res.c_str()));
      }
    else
      {
        throwException ( Exception, "No connect-host attribute !\n" );
      }

    if ( configurationElement.hasAttr(xem_fs.recv_thread_type()))
      {
        String res = configurationElement.getEvaledAttr(xproc,xem_fs.recv_thread_type());
        recvThreadType = stringFromAllocedStr(strdup(res.c_str()));
      }
  }

  XemFSRunCommandService::~XemFSRunCommandService()
  {
    Info ( "Deleting XemFSRunCommandService for host='%s'\n", host.c_str() );
  }

  void
  XemFSRunCommandService::start()
  {
    if ( recvThreadType.size() )
      {
        /*
         *
         */
        if ( recvThreadType == "vlc" )
          {
            startThread ( boost::bind(&XemFSRunCommandService::startRecvThreadVLC,this) );
          }
        else
          {
            throwException ( Exception, "Recv Type not handled : %s\n", recvThreadType.c_str() );
          }
      }
    else
      {
        setStarted();
      }
  }

  void
  XemFSRunCommandService::stop()
  {
    Info ( "Deleting XemFSRunCommandService for host='%s'\n", host.c_str() );
  }

  Document& XemFSRunCommandService::getRecvDocument ()
  {
    if ( recvDocument == NULL )
      {
        throwException(Exception, "No Document defined !\n" );
      }
    return *recvDocument;
  }

  bool
  XemFSRunCommandService::maySelect()
  {
    return ( host.size() );
  }

  void
  XemFSRunCommandService::tryConnect()
  {
    callMutex.assertLocked();
    if (stdinfd != -1)
      {
        Log_CmdInfo ( "Already connected.\n" );
        return;
      }
    if (host.isSpace())
      {
        throwException ( Exception, "Invalid host : %s\n", host.c_str() );
      }
    std::list<String> tokens;
    host.tokenize(tokens, ':');
    if (tokens.size() != 2)
      {
        throwException ( Exception, "Invalid format for host : %s, shall be host_ip:port\n", host.c_str() );
      }
    const char* hostname = tokens.front().c_str();
    const char* portstr = tokens.back().c_str();
    int port = atoi(portstr);
    Log_CmdInfo ( "Connect : hostname=%s, port=%d (%s)\n", hostname, port, portstr );

    int sock;

    if ((sock = ::socket(PF_INET, SOCK_STREAM, 0)) <= -1)
      {
        throwException ( Exception, "Could not connect to '%s:%d' : error = %d:%s\n", hostname, port, errno, strerror(errno) );
      }

    int __sockAddrSz__ = sizeof(struct sockaddr);
    struct sockaddr_in connectionAddress;

    memset(&connectionAddress, 0, sizeof connectionAddress);
    connectionAddress.sin_port = htons(port);
    connectionAddress.sin_family = PF_INET;
    inet_aton(hostname, &(connectionAddress.sin_addr));

    if (connect(sock, (struct sockaddr*) &connectionAddress, __sockAddrSz__)
        == -1)
      {
        ::close ( sock );
        throwException ( Exception, "Could not connect to '%s:%d' : error = %d:%s\n", hostname, port, errno, strerror(errno) );
      }

    Log_CmdInfo ( "Connected to '%s:%d' : sock=%d\n", hostname, port, sock );
    stdinfd = sock;
    stdoutfd = sock;
  }



  void
  XemFSRunCommandService::writeFailed()
  {
    Error ( "Write failed, closing socked %d\n", stdinfd );
    ::close(stdinfd);
    stdinfd = stdoutfd = -1;
  }

  void
  XemFSRunCommandService::readFailed()
  {
    Error ( "Read failed !\n" );
  }

  bool
  XemFSRunCommandService::waitRead(__ui64 waitTime)
  {
    if (!maySelect())
      return true;

    callMutex.lock();
    tryConnect();
    callMutex.unlock();

    int sec = 0;
    int usec = waitTime * 1000;
    fd_set fds;
    FD_ZERO ( &fds );
    FD_SET ( stdoutfd, &fds );

    struct timeval tv =
      { sec, usec };
    int res = ::select(stdoutfd + 1, &fds, NULL, NULL, &tv);
    Log_CmdInfo ( "pselect : clientSocked=%d, res=%d\n", stdoutfd, res );
    if (res == -1)
      {
        Error ( "Could not select: err=%d:%s\n", errno, strerror(errno) );
        readFailed();
        return false;
      }
    if (res <= 0 || !FD_ISSET ( stdoutfd, &fds ))
      {
        return false;
      }
    return true;
  }

  void
  XemFSRunCommandService::sendCommand(const String& cmd)
  {
    Lock lock(callMutex);
    int res;
    int tries = 0;

    do_try: prepareWrite();

    res = ::write(stdinfd, cmd.c_str(), strlen(cmd.c_str()));
    if (res == -1)
      {
        Error ( "Could not write ! err=%d:%s\n", errno, strerror(errno) );
        writeFailed();
        tries++;
        if (tries < maxWriteTries)
          goto do_try;
      }
    else
      {
        Log_CmdInfo ( "Write %d : %s", res, cmd.c_str() );
      }
    fsync(stdinfd);
  }

  __ui64
  jiffies()
  {
    struct timeb tb;
    ftime(&tb);
    return (((__ui64 ) tb.time) * 1000) + ((__ui64 ) tb.millitm);
  }

  String
  XemFSRunCommandService::recvCommand(__ui64 minTime, __ui64 maxTime)
  {
    String result = stringFromAllocedStr(strdup(""));

    Log_CmdInfo ( "recvCommand(%llu,%llu)\n", minTime, maxTime );
    char buffer[4096];

    __ui64 stj = jiffies();
    while (true)
      {
        int res;
          {
            if (waitRead(maxTime))
              {
                res = ::read(stdoutfd, buffer, 4095);
              }
            else
              {
                res = 0;
              }
          }
        Log_CmdInfo ( "RECV : UNLOCKED...\n" );

        if (res == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
          res = 0;
        if (res == 0)
          {
            Log_CmdInfo ( "STDOUT Nothing to read !\n" );
          }
        else if (res == -1)
          {
            Error ( "STDOUT Could not read ! err=%d:%s\n", errno, strerror(errno) );
            ::close(stdoutfd);
            stdoutfd = stdinfd = -1;
            break;
          }
        else
          {
            buffer[res] = '\0';
            result += buffer;
            Log_CmdInfo ( "STDOUT (stj=%llu,now=%llu,minTime=%llu, maxTime=%llu) Read %d bytes : %s\n",
                stj, jiffies(), minTime, maxTime, res, buffer );
          }
        __ui64 now = jiffies();

        if (now - stj >= minTime)
          break;
        usleep(500);
      }
    Log_CmdInfo ( "STDOUT (stj=%llu,now=%llu,minTime=%llu, maxTime=%llu) Read total %lu bytes\n",
        stj, jiffies(), minTime, maxTime, result.size() );
    return result;
  }

};
