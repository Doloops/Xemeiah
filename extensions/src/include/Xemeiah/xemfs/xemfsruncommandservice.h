/*
 * xemfsruncommandservice.h
 *
 *  Created on: 7 nov. 2009
 *      Author: francois
 */

#ifndef __XEM_XEMFSRUNCOMMANDSERVICE_H
#define __XEM_XEMFSRUNCOMMANDSERVICE_H

#include <Xemeiah/xprocessor/xprocessor.h>
#include <Xemeiah/xemprocessor/xemservice.h>
#include <Xemeiah/xemfs/xemfsmodule.h>
#include <Xemeiah/kern/mutex.h>

#include <semaphore.h>
// #define __XEM_XEMFSRUNCOMMANDSERVICE_RUN_FORKED_COMMAND
// #define __XEM_XEMFSRUNCOMMANDSERVICE_SUPERVISER

namespace Xem
{
  /**
   * Run Command Service with in/out/err pipe redirection
   *
   */
  class XemFSRunCommandService : public XemService
  {
    friend class XemFSModule;
  protected:
    /**
     * Reference to our XemFSModule instance
     */
    XemFSModule& xemFSModule;

    /**
     * Reference to the xem-fs builtin namespace
     */
    __BUILTIN_NAMESPACE_CLASS(xem_fs) &xem_fs;

    /**
     * start service
     */
    virtual void start ();

    /**
     * stop service
     */
    virtual void stop ();

    /**
     *
     */
    Mutex callMutex;

    /**
     * Try to connect to remote command
     */
    void tryConnect ();

    /**
     * Prepare to write to remote command
     */
    void prepareWrite () { tryConnect(); }

    /**
     * How much time we should try writing
     */
    static const int maxWriteTries = 5;

    /**
     * Notify that write failed
     */
    void writeFailed ();

    /**
     * Wait to read a certain amount of time
     */
    bool waitRead ( __ui64 waitTime );

    /**
     * Notify that read failed
     */
    void readFailed ();

    /**
     * RecvThread type
     */
    String recvThreadType;

    /**
     * The Document holding status of the receiver handling thread
     */
    Document* recvDocument;

    /**
     * The host to connect to
     */
    String host;


#if 0 // DEPRECATED
    /**
     * The pid of the forked command
     */
    pid_t pid;
#endif

    /**
     * The stdin channel
     */
    int stdinfd;

    /**
     * The stdout channel
     */
    int stdoutfd;

    /**
     * The stderr channel (not available for connect stuff)
     */
    int stderrfd;

    /**
     * Returns to true if we have to perform a select() before reading
     */
    bool maySelect();

    /**
     * Sends this command to stdinfd, adding a trailing '\n' if necessary
     */
    void sendCommand ( const String& cmd );

    /**
     * Try to recieve result from command
     * @param waitTime how much time we should wait, in milliseconds
     */
    String recvCommand ( __ui64 minTime, __ui64 maxTime );

    /**
     * Get the document holding the recv stuff
     */
    Document& getRecvDocument ();

    /**
     *  Do the recieve command stuff
     */
    void doRecvThreadVLC ( ElementRef& status, ElementRef& playlist );

    /**
     * Start the recieve command handler
     */
    void startRecvThreadVLC ( );

  public:
    /**
     * The Command service constructor
     * @param the preliminary XProcessor necessary to evaluate configuration
     */
    XemFSRunCommandService ( XProcessor& xproc, XemFSModule& xemFSModule, ElementRef& configurationElement );

    /**
     * The Command service destructor
     */
    ~XemFSRunCommandService ();
  };

};

#endif // __XEM_XEMFSRUNCOMMANDSERVICE_H
