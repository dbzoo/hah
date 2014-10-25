package com.dbzoo.xaplib;

import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.SocketException;
import java.net.SocketTimeoutException;
import java.net.UnknownHostException;
import java.util.HashMap;
import java.util.logging.Level;
import java.util.logging.Logger;

public class XapManager {

    static XapManager instance;
    static final Logger log = Logger.getLogger(XapManager.class.getName());
    static final int XAP_PORT_L = 3639;
    static final int XAP_PORT_H = 4639;
    static final int XAP_DATA_LEN = 1500;
    String[] source = new String[]{"dbzoo",null,"demo"}; // Triple (Vendor, Host, Instance)
    String uid = "FF123400";
    Frame defaultKeys = null;

    protected XapManager() {
        new Timer(new TimerCallback() {
            public void timerAction(Timer t) {
                send("xap-hbeat\n"
                        + "{\n"
                        + "v=12\n"
                        + "hop=1\n"
                        + "uid=" + getInstance().uid + "\n"
                        + "class=xap-hbeat.alive\n"
                        + "source=" + buildSourceAddress() + "\n"
                        + "interval=60\n"
                        + "}");
            }
        }, 60, true).start();
        setDefaultKeys();
    }

    private static XapManager getInstance() {
        if (instance == null) {
            instance = new XapManager();
        }
        return instance;
    }

    private String buildSourceAddress() {
        if (source[1] == null) {
            source[1] = "xaplib";
            try {
                String result = InetAddress.getLocalHost().getHostName();
                if (!result.isEmpty()) {
                    source[1] = result;
                }
            } catch (UnknownHostException e) {
            }
        }
        return source[0] + "." + source[1] + "." + source[2];
    }

    private void setDefaultKeys() {
        defaultKeys = new Frame("xap-header\n{\nv=12\nhop=1\nsource=" + buildSourceAddress() + "uid=" + uid + "}");
    }

    public static void setSource(String s) {
        getInstance().source[0] = s;
        getInstance().setDefaultKeys();
    }

    public static void setVendor(String s) {
        getInstance().source[1] = s;
        getInstance().setDefaultKeys();
    }

    public static void setInstance(String s) {
        getInstance().source[2] = s;
        getInstance().setDefaultKeys();
    }

    public static void setUID(String uid) {
        getInstance().uid = uid;
        getInstance().setDefaultKeys();
    }

    private static void processLoop(DatagramSocket rx) throws IOException {
        while (true) {
            byte[] buffer = new byte[XAP_DATA_LEN];
            DatagramPacket dp = new DatagramPacket(buffer, buffer.length);
            try {
                rx.receive(dp); // blocking
                byte[] data = dp.getData();
                if (dp.getLength() > 0) {
                    Frame f = new Frame(new String(data, 0, dp.getLength()));
                    FilterManager.dispatch(f);
                }
            } catch (SocketTimeoutException ex) {
                // no response received before timeout, OK.
            }
            TimerManager.dispatch();
        }
    }

    public static void process() {
        DatagramSocket rx = null;
        try {
            try {
                rx = getRxPort();
                rx.setSoTimeout(1000); // ms
                processLoop(rx); // never exited unless an IOException.
            } finally {
                if (rx != null) {
                    rx.close();
                }
            }
        } catch (IOException e) {
            log.log(Level.SEVERE, null, e);
        }
    }

    public static void send(String msg) {
        try {
            DatagramSocket ds = new DatagramSocket();
            InetAddress addr = InetAddress.getByName("255.255.255.255");
            ds.setBroadcast(true);
            byte buffer[] = msg.getBytes();
            DatagramPacket dp = new DatagramPacket(buffer, buffer.length, addr, XAP_PORT_L);
            ds.send(dp);
        } catch (IOException ex) {
            log.log(Level.SEVERE, null, ex);
        }
    }

    public static void sendShort(String msg) {
        Frame f = new Frame(msg);
        f.merge(getInstance().defaultKeys);
        send(f.toString());
    }

    private static DatagramSocket getRxPort() throws SocketException {
        DatagramSocket rx = new DatagramSocket(null);
        rx.setBroadcast(true);
        try {
            rx.bind(new InetSocketAddress(XAP_PORT_L));
            log.log(Level.INFO, "Acquired broadcast socket, port {0}", XAP_PORT_L);
            log.info("Assuming no local hub is active");
        } catch (SocketException bindError) {
            log.log(Level.INFO, "Broadcast socket port {0} in use", XAP_PORT_L);
            log.info("Assuming a hub is active");
            for (int i = XAP_PORT_L + 1; i < XAP_PORT_H; i++) {
                try {
                    rx.bind(new InetSocketAddress(i));
                    log.log(Level.INFO, "Discovered port {0}", i);
                    return rx;
                } catch (SocketException e) {
                    log.log(Level.INFO, "Socket port {0} in use", i);
                }
            }
        }
        return rx;
    }
}
