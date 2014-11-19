/*
 * xemfsruncommandservice-vlc.cpp
 *
 *  Created on: 12 nov. 2009
 *      Author: francois
 */

#include <Xemeiah/kern/volatiledocument.h>
#include <Xemeiah/kern/volatiledocumentallocator.h>
#include <Xemeiah/xemfs/xemfsruncommandservice.h>
#include <Xemeiah/xemfs/xemfsmodule.h>
#include <Xemeiah/xemprocessor/xemservicemodule.h>
#include <Xemeiah/dom/childiterator.h>

#include <Xemeiah/auto-inline.hpp>

#include <time.h>

#include <list>
#include <linux/limits.h>

#define Log_VLC Debug

namespace Xem
{
  /*
   * VLC States
   * 0 : PLAYLIST_STOPPED
   * 1 :
   * 2 :
   * 3 : PLAYLIST_RUNNING
   * 4 : PLAYLIST_PAUSED
   * 5 : PLAYLIST_STOPPED (another one ?)
   *
   */

  const char* VlcPlayStateString[] =
      {
          "Stopped", // 0
          "Unknown 1",
          "Unknown 2",
          "Playing",
          "Paused",
          "Next song", // 5
          "Unknown 6",
          "Unknown 7",
          "Stopped"
      };

  struct VlcStatus
  {
    int playState;
    char inputFile[PATH_MAX];
    int newInput;
    // time_t lastTime;
    time_t lastUpdated;
    time_t length;
    time_t position;
    int lengthOrPosition;
    int dirtyTitle;
    int dirtyPosition;
  };

  void updateCurrentPos ( VlcStatus& vlcStatus, __ui64 pos )
  {
    vlcStatus.lastUpdated = time(NULL);
    // vlcStatus.lastTime = vlcStatus.lastUpdated - pos;
    vlcStatus.position = pos;
  }

  void parseStatus ( const String& statusStr, VlcStatus& vlcStatus )
  {
    std::list<String> lines;
    statusStr.tokenize(lines, '\n');

    if ( lines.size() == 0 )
      {
        Bug ( "." );
      }

    for ( std::list<String>::iterator line = lines.begin() ; line != lines.end() ; line++ )
      {
        const char* s= line->c_str();
        Log_VLC ( "[VLC] Parsing Status line : '%s'\n", s );
        if ( strncmp ( s, "status change: ( ", 17 ) == 0 )
          {
            const char* st = &(s[17]);
            int k = strlen(st)-1;
            for ( ; st[k] != ')' ; k-- ); k--;

            char* strw = (char*) malloc ( k + 1 );
            strncpy ( strw, st, k );
            strw[k] = '\0';



            String statusChange = stringFromAllocedStr( strw );
            Log_VLC ( "[VLC] Status change : %s\n", statusChange.c_str() );
            int oldState = vlcStatus.playState;
            if ( statusChange == "play state: 3" )
              {
                vlcStatus.playState = 3;
              }
            else if ( statusChange == "play state: 8" )
              {
                vlcStatus.playState = 8;
              }
            else if ( statusChange == "stop state: 5" )
              {
                vlcStatus.playState = 5;
              }
            else if ( statusChange == "stop state: 0" )
              {
                vlcStatus.playState = 0;
              }
            else if ( statusChange == "play state: 4" )
              {
                vlcStatus.playState = 4;
              }
            else if ( strncmp ( st, "new input: ", 11 ) == 0 )
              {
                const char* ni = &(statusChange.c_str()[11]);

                strncpy ( vlcStatus.inputFile, ni, PATH_MAX - 1 );
                // vlcStatus.inputFile[k] = '\0';
                vlcStatus.newInput = 1;
                Log_VLC ( "[VLC] NewInput => %s\n", vlcStatus.inputFile);
              }
            else
              {
                Error ( "[VLC] Status change not handled : %s\n", statusChange.c_str() );
              }
            Log_VLC ( "[VLC] State change : %d => %d\n", oldState, vlcStatus.playState );
            if ( oldState != vlcStatus.playState )
              {
                vlcStatus.dirtyTitle = 1;
              }
          }
        else if ( strncmp ( s, "status: ", 8 ) == 0 )
          {

          }
        else if ( strncmp ( s, "play: ", 6 ) == 0 )
          {
            vlcStatus.dirtyTitle = 1;
          }
        else if ( strncmp ( s, "stop: ", 6 ) == 0 )
          {
            /*
             * This is doubtfull : VLC stopped playing the last item, but it may have been starting the next one
             */
            vlcStatus.playState = 8;
            vlcStatus.dirtyTitle = 1;
          }
        else if ( strncmp ( s, "pause: ", 7 ) == 0 )
          {
            // vlcStatus.dirtyTitle = 1;
            if ( vlcStatus.playState == 3 )
              vlcStatus.playState = 4;
            else if ( vlcStatus.playState == 4 )
              vlcStatus.playState = 3;
            else
              {
                Error ( "Invalid playState %d for PAUSE\n", vlcStatus.playState );
              }
          }
        else if ( strncmp ( s, "add: ", 5 ) == 0 )
          {
            vlcStatus.dirtyTitle = 1;
          }
        else if ( strncmp ( s, "goto: ", 6 ) == 0 )
          {
            vlcStatus.dirtyTitle = 1;
          }
        else if ( strncmp ( s, "seek: ", 6 ) == 0 )
          {
            vlcStatus.dirtyTitle = 1;
          }
        else if ( strncmp ( s, "next: ", 6 ) == 0
               || strncmp ( s, "prev: ", 6 ) == 0 )
          {
            vlcStatus.dirtyTitle = 1;
          }
        else if ( strncmp ( s, "Trying to add /", 15 ) == 0 )
          {

          }
        else if ( strncmp ( s, "trying to enqueue /", 19 ) == 0 )
          {

          }
        else if ( strncmp ( s, "enqueue: ", 9 ) == 0 )
          {

          }
        else if ( strncmp ( s, "clear: ", 7 ) == 0 )
          {

          }
        else if ( strncmp ( s, "fastforward: ", 13 ) == 0
               || strncmp ( s, "rewind: ", 8 ) == 0 )
          {
            vlcStatus.dirtyPosition = 1;
          }
        else if ( strncmp ( s, "Tapez ", 6 ) == 0 )
          {
            vlcStatus.dirtyTitle = 1;
          }
        else if ( *s == '\r' )
          {

          }
        else
          {
            if ( ( '0' <= *s ) && ( *s <= '9' ) )
              {
                int res = atoi ( s );
                Log_VLC ( "[VLC] Parsed integer : %d\n", res );
                // First the time, then the length
                if ( vlcStatus.lengthOrPosition == 0 )
                  {
                    Log_VLC ( "[VLC] Set pos=%d\n", res );
                    updateCurrentPos(vlcStatus,res);
                    vlcStatus.lengthOrPosition = 1;
                  }
                else
                  {
                    Log_VLC ( "[VLC] Set length=%d\n", res );
                    vlcStatus.length = res;
                    vlcStatus.lengthOrPosition = 0;

                  }
              }
            else
              {
                Warn ( "[VLC] Not implemented : line = '%s' (*s=%c)\n", s, *s );
              }
          }
      }
    if ( vlcStatus.dirtyTitle )
      {
        Log_VLC ( "[VLC] Leaving with dirtyTitle bit set !\n" );
        vlcStatus.dirtyPosition = 1;
      }
  }

  void XemFSRunCommandService::startRecvThreadVLC ( )
  {
    __BUILTIN_NAMESPACE_CLASS(xem_media) xem_media(getStore().getKeyCache());

    Info ( "Starting VLC thread !\n" );

    AssertBug ( recvDocument == NULL, "Already have a recieve document ?\n" );

    VolatileDocumentAllocator* allocator = new VolatileDocumentAllocator(getStore());
    recvDocument = getStore().createVolatileDocument(*allocator);

    Document& doc = *(recvDocument);
    ElementRef root = doc.getRootElement();
    ElementRef player = doc.createElement(root,xem_media.media_player());
    root.appendChild(player);

    player.addNamespaceAlias(xem_fs.defaultPrefix(),xem_fs.ns());
    player.addNamespaceAlias(xem_media.defaultPrefix(),xem_media.ns());

    ElementRef playlist = doc.createElement(player,xem_media.playlist());
    player.appendChild(playlist);
    ElementRef status = doc.createElement(player,xem_media.status());
    player.appendChild(status);

    status.addAttr(xem_media.play_state(),"Unknown");
    status.addAttrAsInteger(xem_media.length(),0);
    status.addAttrAsInteger(xem_media.position(),0);
    status.addAttrAsInteger(xem_media.title_position(),0);

    status.addAttr(xem_media.title(),"unknown");
    status.addAttr(xem_media.url(),"");

    setStarted();

    while ( isStarting() )
      sleep ( 1 );

    if ( ! isStarted() )
      {
        throwException ( Exception, "Things went wrong here...\n" );
      }


    while ( true )
      {
        Log_VLC ( "[VLC] ** Running doRecvCommandThreadVLC()\n" );
        try
        {
          doRecvThreadVLC ( status, playlist );
        }
        catch ( Exception *e )
        {
          Error ( "Thrown exception : %s\n", e->getMessage().c_str() );
          delete ( e );
        }
        if ( ! isRunning() ) break;
        sleep ( 1 );
      }
  }

  void XemFSRunCommandService::doRecvThreadVLC ( ElementRef& status, ElementRef& playlist )
  {
    __BUILTIN_NAMESPACE_CLASS(xem_media) xem_media(getStore().getKeyCache());

    VlcStatus vlcStatus;
    vlcStatus.newInput = 0;
    vlcStatus.lastUpdated = time(NULL);
    vlcStatus.dirtyTitle = 1;
    vlcStatus.dirtyPosition = 0;
    vlcStatus.playState = 0;
    vlcStatus.length = 0;
    vlcStatus.position = 0;
    vlcStatus.lengthOrPosition = 0;

    sendCommand("status\n");

    __ui64 dirtyTime = 2;
    __ui64 normalTime = 50;
    __ui64 pollTime = 1000;

    while ( true )
      {
        if ( ! isRunning() )
          {
            Warn ( "Service not running, exiting !\n" );
            return;
          }
        String inputQueue = recvCommand(vlcStatus.dirtyTitle ? dirtyTime : normalTime, pollTime);

        Log_VLC ( "[VLC] Update ! (state=%d), (inputQueue size=%lu) lastUpdated age=%d\n",
            vlcStatus.playState, (unsigned long) inputQueue.size(),
            (int) (time(NULL) - vlcStatus.lastUpdated) );

        if ( inputQueue.size() )
          {
            Log_VLC ( "[VLC] Status <= '%s'\n", inputQueue.c_str() );
            parseStatus(inputQueue, vlcStatus);

            AssertBug ( vlcStatus.playState <= 8, "Unknown state %d\n", vlcStatus.playState );
            String playStateStr = VlcPlayStateString[vlcStatus.playState];

            bool mayTriggerChangeCurrentTitle = false;
            if ( status.getAttr(xem_media.play_state()) != playStateStr )
              {
                status.addAttr(xem_media.play_state(),playStateStr);
                mayTriggerChangeCurrentTitle = true;
              }
            if ( vlcStatus.newInput == 1 )
              {
#if 0
                status.addAttr(xem_media.url(),vlcStatus.inputFile);
                String title_ = "Title : ";
                title_ += vlcStatus.inputFile;
                status.addAttr(xem_media.title(),title_);
#endif
                Integer pos = 1;
                bool found = false;
                for ( ChildIterator child(playlist) ; child ; child++ )
                  {
                    if ( child.getAttr(xem_media.url()) == vlcStatus.inputFile )
                      {
                        status.addAttrAsInteger(xem_media.title_position(),pos);
                        Log_VLC ( "[VLC] => Set position to '%lld'\n", pos );
                        found = true;
                        break;
                      }
                    pos++;
                  }
                if ( !found )
                  {
                    Error ( "[VLC] Could not find inputFile '%s' in playlist :\n", vlcStatus.inputFile );
                    for ( ChildIterator child(playlist) ; child ; child++ )
                      {
                        Error ( "\t'%s'\n", child.getAttr(xem_media.url()).c_str());
                      }
                  }
                vlcStatus.newInput = 0;
                mayTriggerChangeCurrentTitle = true;
              }
            if ( mayTriggerChangeCurrentTitle )
              {
                KeyIdList arguments;
                XProcessor& xproc = getPerThreadXProcessor();
                xproc.triggerEvent(xem_media.change_current_title(),arguments);
              }
            if ( vlcStatus.dirtyPosition )
              {
                vlcStatus.dirtyPosition = 0;
                sendCommand("get_time\n");
                sendCommand("get_length\n");
              }
            if ( vlcStatus.dirtyTitle )
              {
                vlcStatus.dirtyTitle = 0;
                sendCommand("status\n");
                continue;
              }
          }

        if ( vlcStatus.playState == 3 )
          {
            time_t now = time(NULL);

            Integer newPosition = vlcStatus.position + now - vlcStatus.lastUpdated;

            /*
             * Position shall not be superior to provided length
             */
            if ( newPosition > vlcStatus.length )
              newPosition = vlcStatus.length;

            AttributeRef positionAttr = status.addAttrAsInteger(xem_media.position(),newPosition);
            AttributeRef lengthAttr = status.addAttrAsInteger(xem_media.length(),vlcStatus.length);

            KeyIdList arguments;
            XProcessor& xproc = getPerThreadXProcessor();

            xproc.setAttribute(xem_media.position(), positionAttr);
            arguments.push_back(xem_media.position());

            xproc.setAttribute(xem_media.length(), lengthAttr);
            arguments.push_back(xem_media.length());

            xproc.triggerEvent(xem_media.change_current_position(),arguments);

            if ( vlcStatus.length == 0 || now - vlcStatus.lastUpdated > 5 )
              {
                sendCommand("get_time\n");
                sendCommand("get_length\n");
              }
          }
      }
  }
};
