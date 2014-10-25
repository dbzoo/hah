package com.dbzoo.xaplib;

import com.dbzoo.xap.Demo;
import java.util.ArrayList;
import java.util.List;
import java.util.Objects;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class Filter {

    public static String FILTER_ANY = "xap_filter_any";
    public static String FILTER_ABSENT = "xap_filter_absent";

    private final List<FilterKey> filterChain = new ArrayList();
    private FilterCallback task;

    public Filter() {
        FilterManager.add(this);
    }

    public void add(String section, String key, String value) {
        filterChain.add(new FilterKey(section, key, value));
    }

    public boolean remove(String section, String key, String value) {
        return filterChain.remove(new FilterKey(section, key, value));
    }

    public boolean isMatch(Frame f) {
        boolean match = true;
        for (FilterKey fk : filterChain) {
            String value = f.getValue(fk.section, fk.key);
            if (fk.value == Filter.FILTER_ABSENT) {
                match = value == null;
            } else if (fk.value == Filter.FILTER_ANY) {
                match = value != null;
            } else if (value == null) {
                match = false;
            } else {
                value = value.toLowerCase();
                if (fk.section.equals("xap-header") && (fk.key.equals("target")
                        || fk.key.equals("source")
                        || fk.key.equals("class"))) {
                    if (fk.key.equals("target")) {
                        match = filterAddrSubaddress(value, fk.value);
                    } else {
                        match = filterAddrSubaddress(fk.value, value);
                    }
                } else {
                    match = value.equals(fk.value);
                }
            }
            if (!match) {
                break;
            }
        }
        return match;
    }

    private boolean filterAddrSubaddress(String filterAddr, String addr) {
        if (filterAddr == null) {
            return true;
        }
        if (!filterAddr.contains("*") && !filterAddr.contains(">")) {
            return filterAddr.equalsIgnoreCase(addr);
        }
        // Convert filterAddr into a regexp
        String s = filterAddr.replace(".", "\\.");
        s = s.replace("*", "[\\w\\-]+");
        s = s.replace(">", ".*");
        Pattern p = Pattern.compile(s);
        Matcher m = p.matcher(addr);
        return m.matches();
    }

    public void callback(FilterCallback task) {
        this.task = task;        
    }
    
    public Runnable callbackFactory(final Frame f) {        
        return new Runnable() {
            @Override
            public void run() {
                task.filterAction(f);
            }
        };
    }
    
    class FilterKey {

        protected String section;
        protected String key;
        protected String value;

        FilterKey(String section, String key, String value) {
            this.section = section;
            this.key = key;
            this.value = value;
        }

        @Override
        public boolean equals(Object other) {
            if (other == null) {
                return false;
            }
            if (other == this) {
                return true;
            }
            if (!(other instanceof FilterKey)) {
                return false;
            }
            FilterKey myOther = (FilterKey) other;
            return myOther.key.equals(this.key) && myOther.section.equals(section) && myOther.value.equals(value);
        }

        @Override
        public int hashCode() {
            int hash = 7;
            hash = 53 * hash + Objects.hashCode(this.section);
            hash = 53 * hash + Objects.hashCode(this.key);
            hash = 53 * hash + Objects.hashCode(this.value);
            return hash;
        }
    }
}
