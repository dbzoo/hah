package com.dbzoo.xap;

import com.dbzoo.xaplib.*;
import java.net.InetAddress;
import java.net.UnknownHostException;

public class Demo {
    public static void main(String args[]) throws UnknownHostException {
        Filter f = new Filter();
        f.add("xap-header", "class", "xapbsc.event");
        f.callback(new FilterCallback() {
            public void filterAction(Frame f) {
                System.out.println("BSC event from " + f.getValue("xap-header", "source"));
            }
        });              
        XapManager.setInstance("demo");
        XapManager.setUID("FF456700");
        XapManager.process();
    }
}
