package com.example.myfirstapp;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.ArrayList;

import android.os.Environment;

public class DiffClient {

    Message rcvMessage;

    public DiffClient(Message rcvMessage) {
	rcvMessage = this.rcvMessage;
    }

    public static void main(String[] args) {}

    /* Compares server and client files to find differences
     * @param rcvMessage The message containing server file info
     * @return A string array containing file names for differences
     */
    public static String[] fileCompare(Message rcvMessage) {

		ArrayList<String> diffList = new ArrayList<String>();
		boolean isFound;
	
		//Get server file names from received message
		String[] serverNames = rcvMessage.getFilenames();
	
		//Get client file names from local directory
		String musicDir = Environment.getExternalStoragePublicDirectory(
			Environment.DIRECTORY_MUSIC).toString();
		File clientFiles = new File(musicDir);
		String[] clientNames = getFiles(clientFiles);
		
		//Get checksums for all client files
		int[] clientSums = new int[clientNames.length];
		for(int i = 0; i < clientNames.length; i++) {
			clientSums[i] = getChecksum(musicDir + "/" + clientNames[i]);
		}
		
		//Get server file checksums
		int[] serverSums = rcvMessage.getCksums();
	
		//Iterate through arrays, comparing server and client checksums
		//and placing server files with no match in diffList array
		for (int i = 0; i < serverSums.length; i++) {
		    isFound = false;
		    for (int j = 0; j < clientSums.length; j++) {
				if (serverSums[i] == clientSums[j]) {
				    isFound = true;
				    break;
				}
		    }
		    if (!isFound) {
		    	diffList.add(serverNames[i]);
		    }
		}
	
		//Transfer song names from array list to string array
		String[] diffNames = new String[diffList.size()];
		for (int i = 0; i < diffNames.length; i++) {
		    diffNames[i] = diffList.get(i).toString();
		}
	
		return diffNames;
    }

    /* Gets file names from a given directory
     * @param folder The directory containing files
     * @return A string array containing all file names in the folder
     */
    public static String[] getFiles(final File folder) {
    	
    	//Initialize an array list for containing the file names
    	ArrayList<String> fileList = new ArrayList<String>();
    	
    	//Iterate through folder to get all file names
        for (final File fileEntry : folder.listFiles()) {
            if (fileEntry.isDirectory()) {
                getFiles(fileEntry);
            } else {
            	fileList.add(fileEntry.getName());	
            }
        }
        
        //Transfer file names from the array list to a string array
        String[] fileNames = new String[fileList.size()];
        for(int i = 0; i < fileNames.length; i++) {
        	fileNames[i] = fileList.get(i).toString();
        }
        return fileNames;
    }
    
    /* Gets a checksum value using MD5 for the given file
     * @param filepath A string containing the path for a file
     * @return The integer checksum of the file's data
     */
    public static int getChecksum(String filepath) {
    	int checksum = 0;
    	try {
    		//Open file input stream for given music file
    		FileInputStream inputStream = new FileInputStream(filepath);
    		
    		//Initialize MD5 digest and byte array for reading data
    		MessageDigest md = MessageDigest.getInstance("MD5");
    		byte[] data = new byte[1024];
    		
    		//Read chunks of bytes into message digest
    		int numRead = 0;
    		while((numRead = inputStream.read(data)) != -1) {
    			md.update(data, 0, numRead);
    		}
    		//Put hashed bytes into a byte array
    		byte[] fileBytes = md.digest();
    		
    		//Add the hashed values, byte by byte to get a checksum
    		for(int i = 0; i < fileBytes.length; i++) {
    			checksum += (fileBytes[i] & 0xFF);
    		}
    		
    		//Close input stream and return calculated checksum
        	inputStream.close();
    		return checksum;
    	}
    	catch(FileNotFoundException fnfe) {
    	}
    	catch(IOException ioe) {
    	}
    	catch(NoSuchAlgorithmException nsae) {
    	}
    	return checksum;
    }

}
