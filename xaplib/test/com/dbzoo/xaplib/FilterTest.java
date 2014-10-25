package com.dbzoo.xaplib;

import org.junit.Test;
import static org.junit.Assert.*;

public class FilterTest {
        private final String xapMsg =        
"xap-header\n"+
"{\n"+
"v=12\n"+
"hop=1\n"+
"uid=FF00D804\n"+
"class=xAPBSC.event\n"+
"source=dbzoo.hahnode.roomnode:rumpus.light\n"+
"}\n"+
"input.state\n"+
"{\n"+
"state=on\n"+
"text=210\n"+
"}"; 
    
    public FilterTest() {        
    }

    @Test
    public void testAdd() {
        System.out.println("add");
        Filter instance = new Filter();
        instance.add("a","b","c");        
    }

    @Test
    public void testDelete() {
        System.out.println("delete");
        Filter instance = new Filter();
        instance.add("a","b","c");        
        assertTrue(instance.remove("a","b","c"));
        assertFalse(instance.remove("a","b","c"));
    }

    @Test
    public void testIsMatch() {
        System.out.println("isMatch");        
        Frame frame = new Frame(xapMsg);        
        Filter filter = new Filter();
        filter.add("xap-header", "class", "xapbsc.event");
        boolean expResult = true;
        boolean result = filter.isMatch(frame);
        assertEquals(expResult, result);
    }
    
    @Test
    public void testIsMatchWildGreater() {
        System.out.println("isMatchWildGreater");
        Frame frame = new Frame(xapMsg);        
        Filter filter = new Filter();
        filter.add("xap-header", "source", "dbzoo.hahnode.>");
        assertTrue(filter.isMatch(frame));
        
        filter = new Filter();
        filter.add("xap-header", "class", "xapTSC.>");
        assertFalse(filter.isMatch(frame));
    }
    
    @Test
    public void testIsMatchWildStar() {
        System.out.println("isMatchWildStar");
        Frame frame = new Frame(xapMsg);        
        Filter filter = new Filter();
        filter.add("xap-header", "source", "dbzoo.hahnode.*:rumpus.light");
        assertTrue(filter.isMatch(frame));
        
        filter = new Filter();
        filter.add("xap-header", "source", "dbzoo.hahnode.*:rumpus.temp");
        assertFalse(filter.isMatch(frame));
        
    }    
}
