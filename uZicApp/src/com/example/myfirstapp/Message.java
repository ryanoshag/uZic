package com.example.myfirstapp;
import java.util.StringTokenizer;

public class Message {
    private static final int MAX_NUM_FILES = 32;
    private String request;
    private int[] cksums = new int[MAX_NUM_FILES];
    private String[] filenames = new String[MAX_NUM_FILES];
    private int len;
    
    public int getLen() {
    	return len;
    }
    
    public String[] getFilenames() {
    	return filenames;
    }
    
    public String[] getFileNamesArray() {
    	return this.filenames;
    }
    
    public int[] getCksums() {
    	return cksums;
    }
    
    public String getRequest() {
    	return request;
    }
    
    public Message(String request, String[] filenames, int[] cksums) {
		this.request = request;
		this.filenames = filenames;
		this.cksums = cksums;
		this.len = filenames.length;
    }
    
    public Message(String request) {
		this.request = request;
		this.filenames = null;
		this.cksums = null;
		this.len = 0;
    }
    
    //For cap, pass cap size as len
    public Message(String request, int len) {
    	this.request = request;
    	this.filenames = null;
    	this.cksums = null;
    	this.len = len;
    }
    
    public static Message decodeMessage(String buffer) {
		StringTokenizer st = new StringTokenizer(buffer, "\t");
		String request = st.nextToken();
		String filenames = st.nextToken();
		String cksums = st.nextToken();
		String temp = st.nextToken();
		int len = Integer.valueOf(temp);
		
		// Populate filenames
		st = new StringTokenizer(filenames, "|");
		String[] fnArr = new String[len];
		for (int i = 0; i < len; i++) {
		    fnArr[i] = st.nextToken();
		}
		
		st = new StringTokenizer(cksums, "|");
		int[] cksumArr = new int[len];
		for (int i = 0; i < len; i++) {
		    cksumArr[i] = Integer.valueOf(st.nextToken());
		}
		return new Message(request, fnArr, cksumArr);
    }
    
    public String toString() {
		String ret = "Message: [REQUEST]: " + this.request + ", [FILENAMES]: ";
		for (int i = 0; i < this.len; i++) {
		    ret += filenames[i] + ", ";
		}
		
		ret += "[CKSUMS]: ";
	
		for (int i = 0; i < this.len; i++) {
		    ret += cksums[i] + ", ";
		}
		
		ret += "[LEN]: " + this.len;
	
		return ret;
    }
    
    public String getFileNameString() {
    	String ret = new String();
    	for (int i = 0; i < this.len; i++) {
    	    ret += filenames[i] + "\n";
    	}
    	return ret;
    }
    
    public static String encodeMessage(Message msg) {
		StringBuilder ret = new StringBuilder();
		ret.append(msg.request);
		ret.append("\t");
				
		if (msg.getLen() == 0 || msg.getRequest().equals("cap")) {
		    ret.append("...\t...");
		}
		else {
		    for (int i = 0; i < msg.getLen(); i++) {
			ret.append(msg.getFilenames()[i]);
			ret.append("|");
		    }
		    
		    ret.append("\t");
	
		    for (int i = 0; i < msg.getLen(); i++) {
			ret.append(msg.getCksums()[i]);
			ret.append("|");
		    }
		}
		
		ret.append("\t");
		ret.append(msg.getLen());
	
		return ret.toString();
	}

}
