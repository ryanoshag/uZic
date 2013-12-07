package com.example.myfirstapp;

import java.io.BufferedWriter;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStreamWriter;
import java.io.PrintWriter;
import java.net.Socket;
import java.net.UnknownHostException;
import java.util.Arrays;
import java.util.concurrent.ExecutionException;

import android.app.Activity;
import android.content.Intent;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Environment;
import android.view.Menu;
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.EditText;
import android.widget.Spinner;
import com.example.uZic.R;

public class MainActivity extends Activity {

    public final static String EXTRA_MESSAGE = "com.example.myfirstapp.MESSAGE";
    public static Socket clientSock = null;
    public Spinner capSpinner;
    public static boolean isConnected = false;
    public static PrintWriter out = null;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
	super.onCreate(savedInstanceState);
	setContentView(R.layout.activity_main);

	capSpinner = (Spinner) findViewById(R.id.cap_spinner);
	ArrayAdapter<CharSequence> adapter = ArrayAdapter.createFromResource(
		this, R.array.cap_array, android.R.layout.simple_spinner_item);
	adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
	capSpinner.setAdapter(adapter);
    }

    @Override
    protected void onResume() {
	super.onResume();
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
	// Inflate the menu; this adds items to the action bar if it is present.
	getMenuInflater().inflate(R.menu.main, menu);
	return true;
    }

    /** Called when the user clicks the Send button */
    public void sendMessage(View view) {
	// Create intent for sending message to DisplayMessageActivity
	Intent intent = new Intent(this, DisplayMessageActivity.class);

	// Get message from text box
	EditText editText = (EditText) findViewById(R.id.edit_message);
	String str = editText.getText().toString();

	// Send message to server and receive transformed message
	String message = null;
	try {
	    message = new TCPSendMessage().execute(str).get();
	} catch (InterruptedException e) {
	    e.printStackTrace();
	} catch (ExecutionException e) {
	    e.printStackTrace();
	}

	// Put message into intent and send to DisplayMessageActivity
	intent.putExtra(EXTRA_MESSAGE, message);
	startActivity(intent);
    }

    private class TCPSendMessage extends AsyncTask<String, Void, String> {

	protected String doInBackground(String... arg0) {
	    try {
		// Create a new socket if not connected already
		if (!isConnected) {
		     clientSock = new Socket("130.207.114.21", 12003);
		    //clientSock = new Socket("192.168.1.68", 12003);
		    out = new PrintWriter(
			    new BufferedWriter(new OutputStreamWriter(
				    clientSock.getOutputStream())), true);
		    isConnected = true;
		}

		// Write the message as an output stream
		/*
		 * PrintWriter out = new PrintWriter(new BufferedWriter( new
		 * OutputStreamWriter(clientSock.getOutputStream())), true);
		 */

		//String capValue = capSpinner.getSelectedItem().toString();

		Message commandMessage;

                if (!capSpinner.getSelectedItem().toString().equals("Data Cap in MB (if applicable)") && arg0[0].equals("cap")) {
                    commandMessage = new Message(arg0[0], Integer.parseInt(capSpinner.getSelectedItem().toString()));
                } 
                else if(capSpinner.getSelectedItem().toString().equals("Data Cap in MB (if applicable)") && arg0[0].equals("cap")) {
                        return "Please select a cap value and try again";
                }
                else if(!(arg0[0].equals("list") || arg0[0].equals("diff") || arg0[0].equals("pull") || arg0[0].equals("leave"))){
                        return "Invalid command";
                }
                else { 
                        commandMessage = new Message(arg0[0]);
                }
		String encodedMessage = new String();
		encodedMessage = Message.encodeMessage(commandMessage);
		out.println(encodedMessage);

		// Receive the message from the server in unsigned bytes
		DataInputStream input = new DataInputStream(
			clientSock.getInputStream());
		StringBuilder str = new StringBuilder();
		for (int i = 0; i < 2048; i++) {
		    str.append((char) input.readUnsignedByte());
		}
		Message rcvMessage = Message.decodeMessage(str.toString());

		if (rcvMessage.getRequest().equals("list")) {
		    return rcvMessage.getFilenames().length
			    + " files on server: \n\n"
			    + rcvMessage.getFileNameString();
		} else if (rcvMessage.getRequest().equals("diff")) {
		    String[] diffNames = DiffClient.fileCompare(rcvMessage);
		    String diffStr = new String();
		    for (int i = 0; i < diffNames.length; i++) {
			diffStr += diffNames[i] + "\n";
		    }
		    return diffNames.length + " unique files on server: \n\n"
			    + diffStr;
		} else if (rcvMessage.getRequest().equals("pull")) {
		    String[] diffNames = DiffClient.fileCompare(rcvMessage);

		    Message sndMessage = new Message("pull", diffNames,
			    rcvMessage.getCksums());
		    encodedMessage = Message.encodeMessage(sndMessage);
		    out.println(encodedMessage);
		    String musicDir = Environment
			    .getExternalStoragePublicDirectory(
				    Environment.DIRECTORY_MUSIC).toString();

		    // Begin huge PULL loop
		    for (int i = 0; i < diffNames.length; i++) {
			// Get file size
			String fileStr = new String();
			char readBuf;
			for (int j = 0; j < 2048; j++) {
			    readBuf = (char) input.readUnsignedByte();
			    if (readBuf != 0)
				fileStr += readBuf;
			    else {
				input.skip(2047 - j);
				break;
			    }
			}

			int fSize = Integer.parseInt(fileStr);

			// Acknowledge file size
			out.println(encodedMessage);

			String fpath = musicDir + "/"
				+ rcvMessage.getFileNamesArray()[i];

			File file = new File(fpath);
			file.createNewFile();

			/*
			 * Writer writer = new BufferedWriter(new
			 * OutputStreamWriter( new FileOutputStream(fpath)));
			 */
			DataOutputStream os = new DataOutputStream(
				new FileOutputStream(fpath));

			// char[] buffer = new char[2048];
			byte[] buffer = new byte[2048];
			int totBytesRead = 0;
			int bytesRead = 0;
			while (totBytesRead < fSize) {
			    Arrays.fill(buffer, (byte) 0);
			    bytesRead = input.read(buffer);
			    totBytesRead += bytesRead;
			    os.write(buffer, 0, bytesRead);
			}
			os.close();

			// Send file done ACK
			out.println(encodedMessage);

		    }

		    // Print diff string
		    String diffStr = new String();
		    for (int i = 0; i < diffNames.length; i++) {

			diffStr += diffNames[i] + "\n";
		    }

		    return diffNames.length + " files pulled from server: \n\n"
			    + diffStr;

		}

		else if (rcvMessage.getRequest().equals("leave")) {
		    clientSock.close();
		    finish();
		    return "Disconnected from server. Bye.";
		}

		else if (rcvMessage.getRequest().equals("cap")) {

		    String[] capNames = rcvMessage.getFilenames();
		    for (int i = 0; i < capNames.length; i++) {
			capNames[i] += ".mp3";
		    }
		    Message sndMessage = new Message("pull", capNames,
			    rcvMessage.getCksums());
		    encodedMessage = Message.encodeMessage(sndMessage);
		    out.println(encodedMessage);
		    String musicDir = Environment
			    .getExternalStoragePublicDirectory(
				    Environment.DIRECTORY_MUSIC).toString();

		    // Begin huge PULL loop
		    String tmp;
		    int fSize;
		    byte[] sizeBuf = new byte[2048];
		    String fpath;
		    StringBuilder fpathBuilder;
		    for (int i = 0; i < capNames.length; i++) {
			Arrays.fill(sizeBuf, (byte) 0);
			input.read(sizeBuf);
			int curByte = 0;
			if (sizeBuf[0] != '0') {
			    while (sizeBuf[curByte] != 0) {
				curByte++;
			    }
			}
			tmp = new String(sizeBuf, 0, curByte, "ASCII");

			fSize = curByte == 0 ? 0 : Integer.parseInt(tmp);

			// Acknowledge file size
			out.println(encodedMessage);

			fpathBuilder = new StringBuilder();
			fpathBuilder.append(musicDir);
			fpathBuilder.append("/");
			fpathBuilder.append(rcvMessage.getFileNamesArray()[i]);

			fpath = fpathBuilder.toString();

			if (fSize > 0) {
			    File file = new File(fpath);
			    file.createNewFile();

			    DataOutputStream os = new DataOutputStream(
				    new FileOutputStream(fpath));

			    byte[] buffer = new byte[2048];
			    int totBytesRead = 0;
			    int bytesRead = 0;
			    while (totBytesRead < fSize) {
				Arrays.fill(buffer, (byte) 0);
				bytesRead = input.read(buffer);
				totBytesRead += bytesRead;
				os.write(buffer, 0, bytesRead);
			    }
			    os.close();
			}

			// Send file done ACK
			out.println(encodedMessage);

		    }

		    // Print cap string
		    String capStr = new String();
		    for (int i = 0; i < capNames.length; i++) {
			capStr += capNames[i] + "\n";
		    }

		    return capNames.length + " files pulled from server: \n\n"
			    + capStr;
		}

		else {
		    return "Unknown command, please try again.";
		}

	    } catch (UnknownHostException e) {
		e.printStackTrace();
	    } catch (IOException e) {
		e.printStackTrace();
		return "Couldn't connect";
	    }
	    return null;
	}

	protected void onPreExecute() {
	}

	protected void onPostExecute(String strOut) {
	}

	protected void onProgressUpdate(Void... values) {
	}

    }

}