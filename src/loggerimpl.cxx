// Module:  Log4CPLUS
// File:    loggerimpl.cxx
// Created: 6/2001
// Author:  Tad E. Smith
//
//
// Copyright 2001-2009 Tad E. Smith
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <log4cplus/internal/internal.h>
#include <log4cplus/spi/loggerimpl.h>
#include <log4cplus/appender.h>
#include <log4cplus/hierarchy.h>
#include <log4cplus/helpers/loglog.h>
#include <log4cplus/spi/loggingevent.h>
#include <log4cplus/spi/rootlogger.h>
#include <stdexcept>


namespace log4cplus { namespace spi {

//////////////////////////////////////////////////////////////////////////////
// Logger Constructors and Destructor
//////////////////////////////////////////////////////////////////////////////
LoggerImpl::LoggerImpl(const log4cplus::tstring& name_, Hierarchy& h)
  : name(name_),
    ll(NOT_SET_LOG_LEVEL),
    parent(NULL),
    additive(true), 
    hierarchy(h)
{
}


LoggerImpl::~LoggerImpl() 
{ 
}


//////////////////////////////////////////////////////////////////////////////
// Logger Methods
//////////////////////////////////////////////////////////////////////////////

void 
LoggerImpl::callAppenders(const InternalLoggingEvent& event)
{
    int writes = 0;
    for(const LoggerImpl* c = this; c != NULL; c=c->parent.get()) {
        writes += c->appendLoopOnAppenders(event);
        if(!c->additive) {
            break;
        }
    }

    // No appenders in hierarchy, warn user only once.
    if(!hierarchy.emittedNoAppenderWarning && writes == 0) {
        getLogLog().error(  LOG4CPLUS_TEXT("No appenders could be found for logger (") 
                          + getName() 
                          + LOG4CPLUS_TEXT(")."));
        getLogLog().error(LOG4CPLUS_TEXT("Please initialize the log4cplus system properly."));
        hierarchy.emittedNoAppenderWarning = true;
    }
}


void 
LoggerImpl::closeNestedAppenders()
{
    SharedAppenderPtrList appenders = getAllAppenders();
    for(SharedAppenderPtrList::iterator it=appenders.begin(); it!=appenders.end(); ++it)
    {
        (*it)->close();
    }
}


bool 
LoggerImpl::isEnabledFor(LogLevel loglevel) const
{
    if(hierarchy.disableValue >= loglevel) {
        return false;
    }
    return loglevel >= getChainedLogLevel();
}


void 
LoggerImpl::log(LogLevel loglevel, 
                helpers::string_param const & message,
                const char* file, 
                int line)
{
    if(isEnabledFor(loglevel)) {
        forcedLog(loglevel, message, file, line);
    }
}



LogLevel 
LoggerImpl::getChainedLogLevel() const
{
    for(const LoggerImpl *c=this; c != NULL; c=c->parent.get()) {
        if(c->ll != NOT_SET_LOG_LEVEL) {
            return c->ll;
        }
    }

    getLogLog().error( LOG4CPLUS_TEXT("LoggerImpl::getChainedLogLevel()- No valid LogLevel found") );
    throw std::runtime_error("No valid LogLevel found");
}


Hierarchy& 
LoggerImpl::getHierarchy() const
{ 
    return hierarchy; 
}


bool 
LoggerImpl::getAdditivity() const
{ 
    return additive; 
}


void 
LoggerImpl::setAdditivity(bool additive_) 
{ 
    additive = additive_; 
}


void 
LoggerImpl::forcedLog(LogLevel loglevel, helpers::string_param const & message,
    const char* file, int line)
{
    spi::InternalLoggingEvent & ev = internal::get_ptd ()->forced_log_ev;
    ev.setLoggingEvent (this->getName(), loglevel, 
        message.is_tstring () ? message.get_tstring () : message.to_tstring (),
        file, line);
    callAppenders(ev);
}


} } // namespace log4cplus { namespace spi {
