import java.io.*;
import java.net.*;
import java.util.*;

public class SocketConn implements Runnable 
{
    SocketConn(Albatross parent)
    {
        albatross = parent;
    }
/***********************************************************/
    public void run()
    {
        String s;
        double cpu,net;
        
        while( bRunning )
        {
            s = get();
            if ( s != null ) 
            {
                StringTokenizer st = new StringTokenizer(s);
    
                if( !st.hasMoreTokens() ) 
                {
                    thread = null;
                    return;
                }

                String hello = st.nextToken ();

                if( !st.hasMoreTokens() ) 
                {
                    thread = null;
                    return;
                }

                String challenge  = st.nextToken ();

                if ( hello.equals( "HELLO" ) )
                {
                    put( "HELLO webStat " + challenge + " c d e\r\n" );
                    break;
                }
            }
        }

        while( bRunning )
        {
            s = get();
            if( s != null )
            {
                albatross.parseStatus( s );
            }
        }
        
        thread = null;
    }

/************************************************************/
    public void connect(String serverHost, int port) throws IOException
    {
        socket = new Socket(serverHost, port);
        input  = new BufferedReader(new InputStreamReader(socket.getInputStream()));
        output = new BufferedWriter(new OutputStreamWriter(socket.getOutputStream()));
        
        bRunning = true;
        thread = new Thread(this);
        thread.start();
    }

/***********************************************************/
    public void disconnect()
    {
        albatross.setMessage("Connection to server lost.");
        bRunning = false;

        try 
        {
            if(socket != null) socket.close();
            socket = null;
            if(input != null) input.close();
            input = null;
            if(output != null) output.close();
            output = null;
        }
        catch(IOException e) 
        {
            System.out.println("Albatross Error IO Exception: " +e);
        }
    
        //if(thread != null) thread.stop();
        //thread = null;
    }

/***********************************************************/
    private void put(String s)
    {
        int len = s.length();
    
        if (len < 1) return;
    
        try 
        {
            output.write(s);
            output.newLine();
            output.flush();
        }
        catch(Exception e) 
        {
            disconnect();
            System.out.println( "Albatross Error when sending to socket: " +e );
        }
    }

/***********************************************************/
    private String get()
    {
        String s;

        try 
        {
            s = input.readLine();
        }
        catch(Exception e) 
        {
            s = null;
        }

        if(s == null) {
            disconnect();
        }

        return(s);
    }

/***********************************************************/
    private Socket socket = null;
    private BufferedReader input = null;
    private BufferedWriter output = null;
    private Thread thread = null;
    private volatile boolean bRunning;
    private Albatross albatross;
}
