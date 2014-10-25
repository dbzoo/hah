package com.dbzoo.xaplib;

import org.junit.After;
import org.junit.AfterClass;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;
import static org.junit.Assert.*;

/**
 *
 * @author brett
 */
public class FrameTest {
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
    
    public FrameTest() {
    }
    
    @BeforeClass
    public static void setUpClass() {
    }
    
    @AfterClass
    public static void tearDownClass() {
    }
    
    @Before
    public void setUp() {
    }
    
    @After
    public void tearDown() {
    }

    /**
     * Test of getValue method, of class Frame.
     */
    @Test
    public void testGetValue() {
        String section = "input.state";
        String key = "state";
        Frame instance = new Frame(xapMsg);              
        String expResult = "on";
        String result = instance.getValue(section, key);
        assertEquals(expResult, result);              
    }
    
    @Test
    public void testGetValueEmptySection() {
        String section = "query";
        String key = null;
        Frame instance = new Frame(
"xap-header\n"+
"{\n"+
"v=12\n"+
"hop=1\n"+
"uid=FF00D804\n"+
"class=xAPBSC.event\n"+
"source=dbzoo.hahnode.roomnode:rumpus.light\n"+
"}\n"+
"query\n"+
"{\n"+
"}");                
        String expResult = Filter.FILTER_ANY;
        String result = instance.getValue(section, key);
        assertEquals(expResult, result);              
    }
    /**
     * Test of isValue method, of class Frame.
     */
    @Test
    public void testIsValue() {
        String section = "xap-header";
        String key = "uid";
        String value = "FF00D804";
        Frame instance = new Frame(xapMsg);
        boolean expResult = true;
        boolean result = instance.isValue(section, key, value);
        assertEquals(expResult, result);
    }

    /**
     * Test of toString method, of class Frame.
     */
    @Test
    public void testToString() {
        Frame instance = new Frame(xapMsg);
        String result = instance.toString();
        assertEquals(instance, new Frame(result));
    }

    @Test
    public void testMerge() {
        Frame f1 = new Frame(xapMsg);
        Frame f2 = new Frame("xap-header\n{test=ok\n}");
        f1.merge(f2);
        String result = f1.getValue("xap-header", "test");
        assertEquals("ok", result);
    }
    
    @Test
    public void testMalformedXap() {
        String s =        
"hello I'm a string\n"+
"v=12"; 
        Frame instance = new Frame(s);
        String result = instance.toString();
        assertEquals(instance, new Frame(result));
    }
}
