// #ifndef __LOG_HPP__
// #define __LOG_HPP__

#pragma once
#include<iostream>
#include<memory>
#include"xlog/xlog.h"


void LogInit(){
    std::unique_ptr<xlog::LoggerBuilder> glb(new xlog::GlobalLoggerBuilder());
    glb->buildLoggerName("mylog");
    glb->buildLoggerType(xlog::LoggerType::LOGGER_ASYNC);
    //glb.buildLoggerType(log::LoggerType::LOGGER_ASYNC); //Default
    // glb.buildEnableUnsafeAsync();
    //glb.buildFormatter(); //Default
    // glb.buildLoggerLevel(xlog::LogLevel::Value::DEBUG); //
    //glb.buildSink<xlog::FileSink>("Tmp/server.log");
    glb->build();
}
// #endif
