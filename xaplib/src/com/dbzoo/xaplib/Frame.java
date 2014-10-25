package com.dbzoo.xaplib;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Objects;

public class Frame {

    enum State {

        START_SECTION_NAME, IN_SECTION_NAME, START_KEYNAME, IN_KEYNAME, START_KEYVALUE, IN_KEYVALUE
    };
    protected Map<String, Map<String, String>> frame = new HashMap();

    Frame(String s) {
        parse(s);
    }

    // Merge keys that don't already exist from f into this Frame
    public void merge(Frame f) {
        for (String section : f.frame.keySet()) {
            Map<String, String> srcKeyPair = f.frame.get(section);
            Map<String, String> dstKeyPair = frame.get(section);
            if (dstKeyPair == null) {
                dstKeyPair = new HashMap();
                frame.put(section, dstKeyPair);
            }
            for (String key : srcKeyPair.keySet()) {
                if (!dstKeyPair.containsKey(key)) {
                    dstKeyPair.put(key, srcKeyPair.get(key));
                }
            }
        }
    }

    public String getValue(String section, String key) {
        Map<String, String> keyPair = frame.get(section);
        if (keyPair == null) {
            return null;
        }
        if (key == null) {
            return Filter.FILTER_ANY;
        }
        return keyPair.get(key);
    }

    public boolean isValue(String section, String key, String value) {
        return getValue(section, key).equals(value);
    }

    private void parse(String s) {
        State state = State.START_SECTION_NAME;
        int idx = 0;
        String key = null;
        Map<String, String> keyPair = null;

        for (int i = 0; i < s.length(); i++) {
            char ch = s.charAt(i);
            switch (state) {
                case START_SECTION_NAME:
                    if ((ch > 32) && (ch < 128)) {
                        state = State.IN_SECTION_NAME;
                        idx = i;
                    }
                    break;
                case IN_SECTION_NAME:
                    if (ch == '{') {
                        String sectionName = s.substring(idx, i).trim().toLowerCase();
                        keyPair = new HashMap();
                        frame.put(sectionName, keyPair);
                        state = State.START_KEYNAME;
                    }
                    break;
                case START_KEYNAME:
                    if (ch == '}') {
                        state = State.START_SECTION_NAME;
                    } else if ((ch > 32) && (ch < 128)) {
                        idx = i;
                        state = State.IN_KEYNAME;
                    }
                    break;
                case IN_KEYNAME:
                    if ((ch < 32) || (ch == '=')) {
                        key = s.substring(idx, i).trim().toLowerCase();
                        state = State.START_KEYVALUE;
                    }
                    break;
                case START_KEYVALUE:
                    if ((ch > 32) && (ch < 128)) {
                        state = State.IN_KEYVALUE;
                        idx = i;
                    }
                    break;
                case IN_KEYVALUE:
                    if (ch < 32) {
                        String value = s.substring(idx, i);
                        keyPair.put(key, value);
                        state = State.START_KEYNAME;
                    }
                    break;
            }
        }
    }

    @Override
    public boolean equals(Object other) {
        if (other == null) {
            return false;
        }
        if (other == this) {
            return true;
        }
        if (!(other instanceof Frame)) {
            return false;
        }
        Map<String, Map<String, String>> myOther = ((Frame) other).frame;
        for (String section : frame.keySet()) {
            Map otherKeyPair = myOther.get(section);
            if (otherKeyPair == null) {
                return false;
            }
            Map keyPair = frame.get(section);
            if (!keyPair.equals(otherKeyPair)) {
                return false;
            }
        }
        return true;
    }

    @Override
    public int hashCode() {
        int hash = 7;
        hash = 19 * hash + Objects.hashCode(this.frame);
        return hash;
    }

    private String encodeOrderedHeader(String section) {
        StringBuilder s = new StringBuilder();
        Map<String, String> keyPair = frame.get(section);
        String[] orderKeys = {"v", "hop", "uid", "class", "source", "target"};
        List<String> foundKeys = new ArrayList();
        s.append(section);
        s.append(System.lineSeparator());
        s.append("{");
        s.append(System.lineSeparator());
        for (String key : orderKeys) {
            String v = getValue(section, key);
            if (v != null) {
                s.append(key);
                s.append("=");
                s.append(v);
                s.append(System.lineSeparator());
                foundKeys.add(key);
            }
        }
        // Append all other header keys
        for (String key : keyPair.keySet()) {
            if (!foundKeys.contains(key)) {
                s.append(key);
                s.append("=");
                s.append(keyPair.get(key));
                s.append(System.lineSeparator());
            }
        }
        s.append("}");
        s.append(System.lineSeparator());
        return s.toString();
    }

    @Override
    public String toString() {
        StringBuilder s = new StringBuilder();
        // Sequence the xap-header's first for non-compliant xAP implementations.
        // HomeSeer (The developer of xAP for this should hang their head low)
        String header = null;
        for (String k : new String[]{"xap-header", "xap-hbeat"}) {
            if (frame.containsKey(k)) {
                header = k;
                s.append(encodeOrderedHeader(k));
                break;
            }
        }

        for (String section : frame.keySet()) {
            if (header != null && header.equals(section)) {
                continue;
            }
            s.append(section);
            s.append(System.lineSeparator());
            s.append("{");
            s.append(System.lineSeparator());
            Map<String, String> keyPair = frame.get(section);
            for (String key : keyPair.keySet()) {
                s.append(key);
                s.append("=");
                s.append(keyPair.get(key));
                s.append(System.lineSeparator());
            }
            s.append("}");
            s.append(System.lineSeparator());
        }
        return s.toString().trim();
    }
}
