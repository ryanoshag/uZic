package com.example.myfirstapp;

import java.io.BufferedWriter;
import java.io.OutputStreamWriter;
import java.io.PrintWriter;
import java.net.Socket;

import android.app.Service;
import android.content.Intent;
import android.os.Binder;
import android.os.IBinder;
import android.util.Log;

public class SocketService extends Service {

	public static final String SERVERIP = "130.207.114.21";
	public static final int SERVERPORT = 12003;
	Socket clientSock;
	PrintWriter out;
	private final IBinder socketBinder = new SocketBinder();
	
	
	public class SocketBinder extends Binder {
		public SocketService getService() {
			return SocketService.this;
		}
	}

	@Override
	public IBinder onBind(Intent intent) {
		return socketBinder;
	}
	
	public void onCreate() {
		super.onCreate();
		Log.d("debugging", "In on create");
	}
	
	public String sendMessage(String message) {
		if(out != null &&!out.checkError()) {
			try {
				out = new PrintWriter(new BufferedWriter(new OutputStreamWriter(clientSock.getOutputStream())), true);
				Message commandMessage = new Message(message);
				String encodedMessage = Message.encodeMessage(commandMessage);
				out.println(encodedMessage);
				out.flush();
			}
			catch(Exception e) {
				Log.e("debugging", "Error with sending message to server", e);
			}
		}
		Log.d("debugging", "Message sent");
		return "Message sent";
	}
	
	public int onStartCommand(Intent intent, int flags, int startId) {
		super.onStartCommand(intent, flags, startId);
		Log.d("debugging", "In onStartCommand");
		Runnable connect = new connectSocket();
		new Thread(connect).start();
		return START_STICKY;
	}
	
	class connectSocket implements Runnable {
		
		public void run() {
			try { 
               clientSock = new Socket(SERVERIP, SERVERPORT);
               Log.d("debugging", "Connecting to server");
                try {
                    //send the message to the server
                	Log.d("debugging", "Sending message to server");
                    out = new PrintWriter(new BufferedWriter(new OutputStreamWriter(clientSock.getOutputStream())), true);
                } 
                catch (Exception e) {
                    Log.e("debugging", "Error with sending message to server", e);
                }
			} 
			catch (Exception e) {
               Log.e("debugging", "Error with creating socket", e);
           }
       }
   }

   @Override
   public void onDestroy() {
	   super.onDestroy();
       try {
    	   clientSock.close();
       } 
       catch (Exception e) {
    	   Log.e("debugging", "Error with closing socket", e);
       }
       clientSock = null;
   }
}
