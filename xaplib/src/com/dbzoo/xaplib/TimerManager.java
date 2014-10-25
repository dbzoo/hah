package com.dbzoo.xaplib;

import java.util.ArrayList;
import java.util.List;

class TimerManager {
    private static TimerManager instance = null;
    private final List<Timer> timerList = new ArrayList();
    protected TimerManager() {      
    }
    public static TimerManager getInstance() {
        if(instance == null) {
            instance = new TimerManager();
        }
        return instance;
    }
    
    public static void add(Timer t) {
        getInstance().timerList.add(t);
    }
    
    public static void dispatch() {
        for(Timer timer: getInstance().timerList) {
            if(timer.expired()) {
                Runnable r = timer.callbackFactory();
                new Thread(r).start();  
            }
        }
    }
    
}
