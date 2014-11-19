#include <Xemeiah/kern/branchmanager.h>
#include <Xemeiah/kern/exception.h>

#include <stdio.h>

#include <Xemeiah/auto-inline.hpp>

#define Log_BM Debug

namespace Xem
{
  BranchRevId BranchManager::parseBranchRevId ( const char* branchRevStr )
  {
    BranchRevId branchRevId;
    int res = sscanf ( branchRevStr, "%llu:%llu", &(branchRevId.branchId), &(branchRevId.revisionId) );
    if ( res != 2 )
      {
        branchRevId.branchId = 0;
        branchRevId.revisionId = 0;
      }
    return branchRevId;  
  }

  static const char* documentFlagStrings[] =
  {
    "not-set", "read", "explicit-read", "write", "reuse-write", "as-revision", "unmanaged", "reset-revision", "follow-branch", NULL
  };

  DocumentOpeningFlags BranchManager::parseDocumentOpeningFlags ( const String& flagsStr )
  {
    for ( int flg = DocumentOpeningFlags_NotSet ; flg < DocumentOpeningFlags_Invalid ; flg++ )
      {
        if ( flagsStr == documentFlagStrings[flg] )
          return (DocumentOpeningFlags)flg;
      }
    throwException ( Exception, "Invalid flags string '%s'\n", flagsStr.c_str() );
  }

  const char* BranchManager::getDocumentOpeningFlagsName ( DocumentOpeningFlags flags )
  {
    if ( flags > DocumentOpeningFlags_Invalid )
      flags = DocumentOpeningFlags_NotSet;
    return documentFlagStrings[flags];
  }
      
  Document* BranchManager::openDocument ( const String& branchName, const String& flagsStr, RoleId roleId )
  {
    BranchId branchId = 0;
    RevisionId revisionId = 0;

    BranchRevId brId = parseBranchRevId ( branchName.c_str() );
    if ( brId.branchId )
      {
        branchId = brId.branchId;
        revisionId = brId.revisionId;
      }
    else
      {
        branchId = getBranchId ( branchName.c_str() );
        if ( ! branchId )
          {
            throwException ( Exception, "Could not lookup branch name '%s'\n", branchName.c_str() );
          }
        revisionId = 0;
      }

    DocumentOpeningFlags flags = parseDocumentOpeningFlags ( flagsStr );

    Document* document = openDocument ( branchId, revisionId, flags, roleId );
    
    if ( ! document )
      {
        throwException ( Exception, "Could not open document !\n" );
      }
    return document;
  }

  Document* BranchManager::openDocument ( BranchId branchId, RevisionId revisionId, DocumentOpeningFlags flags )
  {
    return openDocument ( branchId, revisionId, flags, getStore().getKeyCache().getBuiltinKeys().nons.none() );
  }

  static const char* branchFlagNames[] =
  { "must-fork-before-update", 
    "may-index-name-if-duplicate", 
    "auto-forked", 
    "auto-merge",
    "may-not-be-xupdated", 
    "can-fork-at-reopen",
    NULL };

  BranchId BranchManager::createBranch ( const String& branchName, BranchRevId forkedFrom, const String& branchFlagsStr )
  {
    BranchFlags branchFlags = 0;
    std::list<String> tokens;
    branchFlagsStr.tokenize(tokens);
    for ( std::list<String>::iterator token = tokens.begin() ; token != tokens.end() ; token++ )
      {
        String& flag = *token;
        int i;
        for ( i = 0 ; branchFlagNames[i] ; i++ )
          {
            if ( flag == branchFlagNames[i] )
              {
                branchFlags |= 1<<i;
                Log_BM ( "BranchFlags : found flag='%s', i=%d, flag=0x%llx\n", flag.c_str(), i, branchFlags );
                break;
              }
          }
        if ( !branchFlagNames[i] )
          {
            throwException ( Exception, "Invalid flag '%s' from '%s'\n", flag.c_str(), branchFlagsStr.c_str() );
          }
      }
    return createBranch ( branchName, forkedFrom, branchFlags );
  }

};

