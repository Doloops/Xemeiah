#if 0 // DEPRECATED
/*
 * xemfsruncommandservice-fork.cpp
 *
 *  Created on: 17 nov. 2009
 *      Author: francois
 */

  void
  XemFSModule::instructionRunCommand(__XProcHandlerArgs__)
  {
#ifdef __XEM_XEMFSRUNCOMMANDSERVICE_RUN_FORK
    XemServiceModule& xemServiceModule = XemServiceModule::getMe(
        getXProcessor());
    String serviceName = item.getEvaledAttr(getXProcessor(),
        xemServiceModule.xem_service.name());
    XemFSRunCommandService& service = getXemFSRunCommandService(serviceName);

    String identifier =
        item.getEvaledAttr(getXProcessor(), xem_fs.identifier());
    String command = item.getEvaledAttr(getXProcessor(), xem_fs.command());

    Info ( "Running command '%s'\n", command.c_str() );

    std::list<String> arguments;
    for (ChildIterator arg(item); arg; arg++)
      {
        if (arg.getKeyId() != xem_fs.argument())
          continue;
        String value = arg.getEvaledAttr(getXProcessor(), xem_fs.value());
        Info ( "\tArgument '%s'\n", value.c_str() );
        arguments.push_back(value);
      }
    service.runCommand(identifier, command, arguments);
    /**
     * Keep a grace period for fork() to act properly..
     */
    usleep(1000);
#else
    throwException(Exception,"xem-fs:run-command : __XEM_XEMFSRUNCOMMANDSERVICE_RUN_FORK not set !\n" );
#endif // __XEM_XEMFSRUNCOMMANDSERVICE_RUN_FORK
  }

#ifdef __XEM_XEMFSRUNCOMMANDSERVICE_RUN_FORKED_COMMAND
  void
  XemFSRunCommandService::runCommand(const String& identifier,
      const String& command, std::list<String>& arguments)
  {
    Log_CmdInfo ( "runCommand(%s,%s,#%lx arguments)\n", identifier.c_str(), command.c_str(), (unsigned long) arguments.size() );

    CommandInfo& commandInfo = createCommandInfo(identifier);

    int res;

    Log_CmdInfo ( "FILENO: in=%d, out=%d, err=%d\n", STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO );

    int stdinpipe[2];
    res = pipe2(stdinpipe, 0); // O_NONBLOCK );
    if (res)
      {
        Bug ( "Could not create pipes : err=%d:%s\n", errno, strerror(errno) );
      }

    int stdoutpipe[2];
    res = pipe2(stdoutpipe, O_NONBLOCK);
    if (res)
      {
        Bug ( "Could not create pipes : err=%d:%s\n", errno, strerror(errno) );
      }

    int stderrpipe[2];
    res = pipe2(stderrpipe, O_NONBLOCK);
    if (res)
      {
        Bug ( "Could not create pipes : err=%d:%s\n", errno, strerror(errno) );
      }

    commandInfo.stdinfd = stdinpipe[1];
    commandInfo.stdoutfd = stdoutpipe[0];
    commandInfo.stderrfd = stderrpipe[0];

    Log_CmdInfo ( "Before fork : stdinfd=%d, stdoutfd=%d\n", commandInfo.stdinfd, commandInfo.stdoutfd );

    pid_t child = fork();
    if (child == -1)
      {
        throwException ( Exception, "Could not fork : error=%d:%s\n", errno, strerror(errno) );
      }
    if (child == 0)
      {
        child = getpid();
        Log_CmdInfo ( "At child %x !\n", child );

        const char** argv = (const char**) malloc(sizeof(char*)
            * (arguments.size() + 2));
        int argc = 0;
        argv[argc++] = command.c_str();
        for (std::list<String>::iterator iter = arguments.begin(); iter
            != arguments.end(); iter++)
          {
            argv[argc++] = iter->c_str();
          }
        Log_CmdInfo ( "Argc = %d\n", argc );
        argv[argc] = NULL;
        for (int i = 0; i < argc; i++)
          {
            Log_CmdInfo ( "\tArg[%d]=%s\n", i, argv[i] );
          }

        Log_CmdInfo ( "Inside fork : stdin<-%d, stdout<-%d, stderrpipe<-%d\n", stdinpipe[0], stdoutpipe[1], stderrpipe[1] );

        res = dup2(stdinpipe[0], STDIN_FILENO);
        if (res == -1)
          {
            Bug ( "Could not dup2 STDIN : err=%d:%s\n", errno, strerror(errno) );
          }
        ::close(stdinpipe[1]);

        res = dup2(stdoutpipe[1], STDOUT_FILENO);
        if (res == -1)
          {
            Bug ( "Could not dup2 STDOUT : err=%d:%s\n", errno, strerror(errno) );
          }
        ::close(stdoutpipe[1]);

        res = dup2(stderrpipe[1], STDERR_FILENO);
        if (res == -1)
          {
            Bug ( "Could not dup2 STDERR : err=%d:%s\n", errno, strerror(errno) );
          }
        ::close(stderrpipe[1]);

        res = execvp(command.c_str(), (char* const *) argv);
        Error ( "Execvp error : command=%s, res=%d, err=%d:%s\n",
            command.c_str(), res, errno, strerror(errno) );
      }
    else if (child)
      {
        Log_CmdInfo ( "Spawn new child : at %x, waiting...\n", child );
        Log_CmdInfo ( "Spawn new child : in=%d, out=%d...\n", commandInfo.stdinfd, commandInfo.stdoutfd );

        commandInfo.pid = child;
      }
  }
#endif


#ifdef __XEM_XEMFSRUNCOMMANDSERVICE_SUPERVISER
  void
  XemFSRunCommandService::runSuperviserThread(void* arg)
  {
    setStarted();

    Log_ST ( "Superviser Thread !\n" );

    while (true)
      {
        sleep(1);
        Log_ST ( "Superviser LOCKING...\n" );

        Lock lock(commandMutex);
        Log_ST ( "Superviser LOCKED !\n" );
        for (CommandsMap::iterator iter = commandsMap.begin(); iter
            != commandsMap.end(); iter++)
          {
            AssertBug ( iter->second, "Null iterator !\n" );
            CommandInfo& info = *(iter->second);
            Log_ST ( "Command at %p\n", &info );
            Log_ST ( "identifier at %p\n", info.identifier.c_str() );
            Log_ST( "Command '%s', pid %x, fd=%d/%d/%d/%s\n", info.identifier.c_str(), info.pid,
                info.stdinfd, info.stdoutfd, info.stderrfd, info.host.c_str() );

            pid_t child = info.pid;
            if (child)
              {
                int status;
                int options = WNOHANG;
                pid_t waited = 0;

                waited = waitpid(child, &status, options);
                if (waited == -1)
                  {
                    Error ( "Could not wait pid !\n" );
                  }
                else if (waited == 0)
                  {
                    Log_ST ( "No news from child ?\n" );
                  }
                else if (waited == child)
                  {
                    Info ( "Child dead, killing child !\n" );
                    // iter = commandsMap.erase(iter);
                    commandsMap.erase(iter);
                  }
                else
                  {
                    Error ( "Supurious return of waitpid(%x) => %x\n", child, waited );
                  }
              }
          }
        if (!isStarted())
          {
            Info ( "Exiting superviser thread !\n" );
            break;
          }
      }
    Info ( "Superviser Ends !\n" );
  }
#endif // __XEM_XEMFSRUNCOMMANDSERVICE_SUPERVISER



#endif
