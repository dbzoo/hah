package com.dbzoo.xaplib;

public class Timer {
    private long ttl = 0;
    private final TimerCallback task;
    private boolean isAlive = false;
    private boolean initial;
    private long interval; // seconds
    
    public Timer(TimerCallback task, long interval, boolean initial) {
        this.task = task;
        this.initial = initial;
        this.interval = interval * 1000; // Seconds to ms
        TimerManager.add(this);
    }
    public Timer(TimerCallback task, long interval) {
        this(task, interval, false);
    }
    
    public Runnable callbackFactory() {        
        final Timer t = this;
        return new Runnable() {
            @Override
            public void run() {
                t.reset();
                task.timerAction(t);
            }
        };
    }

    public boolean expired() {
        return ttl <= System.currentTimeMillis() && isAlive;
    }
    
    public void stop() {
        isAlive = false;
    }
    
    public void start() {
        isAlive = true;
        if(initial) {
            ttl = 0;
        } else {
            reset();
        }
    }
    
    public void reset() {
        ttl = System.currentTimeMillis() + interval;
    }  
}
