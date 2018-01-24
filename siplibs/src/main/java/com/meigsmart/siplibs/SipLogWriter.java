package com.meigsmart.siplibs;

import org.pjsip.pjsua2.LogEntry;
import org.pjsip.pjsua2.LogWriter;

/**
 * Created by chenMeng on 2018/1/11.
 */

public class SipLogWriter extends LogWriter {

    @Override
    public void write(LogEntry entry) {
        String log = entry.getMsg();
        if (entry.getLevel() == 1){
            if (log.contains("Operation not permitted [status=120001]")){
                System.out.print("Operation not permitted,start help activity");
            }
        }
        System.out.print(log);
    }
}
