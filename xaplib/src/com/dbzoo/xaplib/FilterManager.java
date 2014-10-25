package com.dbzoo.xaplib;

import java.util.ArrayList;
import java.util.List;

public class FilterManager {
    private static FilterManager instance = null;
    private final List<Filter> filterList = new ArrayList();
    protected FilterManager() {      
    }
    public static FilterManager getInstance() {
        if(instance == null) {
            instance = new FilterManager();
        }
        return instance;
    }
    
    public static void add(Filter f) {
        getInstance().filterList.add(f);
    }
    
    public static void dispatch(Frame f) {
        for(Filter filter: getInstance().filterList) {
            if(filter.isMatch(f)) {
                Runnable r = filter.callbackFactory(f);
                new Thread(r).start();  
            }
        }
    }
}
