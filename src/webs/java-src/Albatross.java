import java.awt.*;
import java.applet.*;
import java.io.*;
import java.net.*;
import java.util.*;

public class Albatross extends Applet {

    private Font font;
    
    private String webPhoneURL,frame, serverHost,message;
    private int tcpPort,iPorts,maxPorts,iRefreshRate, dspCount;
    private int imgBg_width, imgBg_height;
    private Image imgBg;
    private Image imgPorts[];
    private MediaTracker tracker;
    private Label phonenr[];
    private PhonePort port[];
    private GraphView graphCPU,graphDSP;
    private SocketConn conn;

/***********************************************************/
    public void init() 
    {
        message = "Not connected, wait...";
        int imgPorts_height, imgPorts_width;
        String temp;
        String imgpath = getParameter("ImagePath");
        if (imgpath==null)
            imgpath = "";

        tracker = new MediaTracker(this);
        imgPorts = new Image[6];
        imgPorts[0] = getImage(getDocumentBase(),imgpath +"rj45_lsd_down.gif");
        tracker.addImage(imgPorts[0], 0);
        imgPorts[1] = getImage(getDocumentBase(),imgpath +"rj45_elu_down.gif");
        tracker.addImage(imgPorts[1], 1);
        imgPorts[2] = getImage(getDocumentBase(),imgpath +"rj45_ok.gif");
        tracker.addImage(imgPorts[2], 2);
        imgPorts[3] = getImage(getDocumentBase(),imgpath +"rj45_ringing.gif");
        tracker.addImage(imgPorts[3], 3);
        imgPorts[4] = getImage(getDocumentBase(),imgpath +"rj45_transmission.gif");
        tracker.addImage(imgPorts[4], 4);
        imgPorts[5] = getImage(getDocumentBase(),imgpath +"rj45_falty_dts.gif");
        tracker.addImage(imgPorts[5], 5);

        imgBg = getImage(getDocumentBase(),imgpath +"bg_java.gif");
        tracker.addImage(imgBg, 7);
        
        setBackground(Color.lightGray);
        font = new Font("Helvetica",font.BOLD,14);

        URL codebase = getCodeBase();
        serverHost = codebase.getHost();

        tcpPort = 7077;
        temp = getParameter("tcpPort");
        if(temp != null) 
            tcpPort = Integer.parseInt(temp);

        webPhoneURL = getParameter("WebPhoneURL");

        frame = getParameter("Frame");
        if(frame == null)
            frame = "_self";
            
        iRefreshRate = 2000;
        temp = getParameter("RefreshRate");
        if(temp != null) 
        {
            iRefreshRate = Integer.parseInt(temp);
            if(iRefreshRate < 2000) iRefreshRate = 2000;
        }

        maxPorts = 8;
        iPorts = 0;
        temp = getParameter("NrOfPorts");
        if(temp != null) 
        {
            iPorts = Integer.parseInt(temp);
            if( iPorts <= 0 ) iPorts = 0;
            if( iPorts > maxPorts ) iPorts = maxPorts;
        }
        dspCount = iPorts/2;
        
        try 
        {  
            tracker.waitForAll();
        }
        catch (InterruptedException e) 
        { 
            System.out.println( "Albatross Error loading images: " +e );
            this.showStatus( "Albatross Error loading images: " +e  );
            setMessage( "Loading images failed: " +e );
        }
        
        imgBg_height = imgBg.getHeight(null);
        imgBg_width = imgBg.getWidth(null);
        imgPorts_height = imgPorts[0].getHeight(null);
        imgPorts_width = imgPorts[0].getWidth(null);

        GridBagLayout gbl = new GridBagLayout();
        setLayout(gbl);

        GridBagConstraints gbc = new GridBagConstraints();
        gbc.gridwidth = 1;
        gbc.gridheight = 4;
        gbc.weightx = 0;
        gbc.weighty = 100;
        gbc.gridx = gbc.gridy = 0;
        gbc.anchor = gbc.NORTH;
        gbc.fill = gbc.BOTH;
        gbc.insets = new Insets(4, 4, 14, 4);

        graphCPU = new GraphView(120,getSize().height-8, 4, "Host");
        graphCPU.setColor(0,Color.white);
        graphCPU.setColor(1,Color.green);
        graphCPU.setColor(2,Color.red);
        graphCPU.setColor(3,Color.yellow);
        graphCPU.setUnit(0,"%");
        graphCPU.setUnit(1,"MiB");
        graphCPU.setUnit(2,"kbps");
        graphCPU.setUnit(3,"kbps");
        graphCPU.setMaxValue(0,1000); // 100%
        graphCPU.setMaxValue(1,640); // 64MB
        graphCPU.setMaxValue(2,5120); // 512kbps
        graphCPU.setMaxValue(3,5120); // 512kbps
        gbl.setConstraints(graphCPU, gbc);
        add(graphCPU);

        gbc.gridx++;
        graphDSP = new GraphView(120,getSize().height-8, dspCount, "DSP");
        for ( int i = 0; i < dspCount; i++ )
        {
            graphDSP.setColor(i, i == 0 ? Color.white : i == 1 ? Color.green : i == 2 ? Color.cyan : Color.blue );
            graphDSP.setUnit(i,"%");
            graphDSP.setMaxValue(i,1000);
        }

        gbl.setConstraints(graphDSP, gbc);
        add(graphDSP);
        
        port = new PhonePort[maxPorts];
        phonenr = new Label[maxPorts];
        gbc.weightx = 100;
        gbc.weighty = 0;
        gbc.gridheight = 1;
        gbc.fill = gbc.NONE;
        gbc.insets = new Insets(4, 12, 0, 4);

        Cursor cursor = new Cursor(Cursor.HAND_CURSOR);

        for(int i = 0; i < maxPorts; i++) 
        {
            port[i] = new PhonePort(null,imgPorts_width,imgPorts_height,i,this);

            if( i < iPorts )
            {
                phonenr[i] = new Label();
                phonenr[i].setFont(font);
                temp = getParameter("Port" +Integer.toString(i));
                if(temp != null) 
                {
                    phonenr[i].setText(temp);
                    port[i].setCursor(cursor);
                }
                else
                {
                    phonenr[i].setText( "Port " + Integer.toString(i+1) );
                }
                phonenr[i].setAlignment( Label.CENTER ); // CENTER
            }

            gbc.gridx++;


            if( i < iPorts )
            {
                gbl.setConstraints(phonenr[i], gbc);
                add(phonenr[i]);        
            }

            gbc.weighty = 100;
            gbc.gridy++;

            gbl.setConstraints(port[i], gbc);
            add(port[i]);

            gbc.weighty = 0;
            gbc.gridy--;
        }

        conn = new SocketConn(this);
    }

/***********************************************************/
    public void start() 
    {
        try 
        {   
            conn.connect(serverHost,tcpPort);
            setMessage("DEX28: Ready.");
        }
        catch(IOException e) 
        {
            System.out.println( "Albatross Error connecting: " + e );
            setMessage("Connection failed.");
        }
    }

/***********************************************************/
    public void stop() 
    {
        conn.disconnect();
    }

/***********************************************************/
    public void destroy() 
    {
    
    }
    
/***********************************************************/
    public void update(Graphics g) 
    {
        paint(g);
    }

/***********************************************************/
    public void paint(Graphics g) 
    {
        Dimension d = getSize();
        
        Image imgCanvas = createImage( d.width, d.height );
        Graphics graph = imgCanvas.getGraphics();
        
        Font f = new Font("Helvetica",Font.PLAIN,10);
        graph.setFont(f);
        graph.drawString( message, 6, d.height-4 );
        
        graph.dispose();
        
        g.drawImage(imgCanvas,0,0,this);
    }

/***********************************************************/
    public void setMessage(String s)
    {
        message = s;
        repaint();  
    }
    
/***********************************************************/
    public void parseStatus(String s)
    {
        String portStatus;
        StringTokenizer st = new StringTokenizer(s);
    
        if( !st.hasMoreTokens() ) 
        {
            System.out.println( "Albatross Error no more tokens when parsing" );
            this.showStatus( "Albatross Error no more tokens when parsing" );
            return;
        }

        int portc = 0;

        portStatus = st.nextToken();
        for( int i = 0; i < portStatus.length() && i < maxPorts; i++ ) 
        {
            char temp = portStatus.charAt( i );
            if ( temp == 'X' ) // end of ports; usefull when portc = 0
                break;

            ++portc;

            switch( temp) 
            {
                case '-':
                    port[i].loadImage(imgPorts[0]);
                    break;
                case '?':
                    port[i].loadImage(imgPorts[1]);
                    break;
                case '*':
                    port[i].loadImage(imgPorts[2]);
                    break;
                case 'r':
                case 'R':
                    port[i].loadImage(imgPorts[3]);
                    break;
                case 't':
                case 'T':
                    port[i].loadImage(imgPorts[4]);
                    break;
                case 'f':
                case 'F':
                    port[i].loadImage(imgPorts[5]);
                    break;
                default:
                    port[i].loadImage(imgPorts[1]);
                    break;
            }
        }
        
        try 
        {
            int cpu = Integer.parseInt(st.nextToken());
            int mem_used = Integer.parseInt(st.nextToken()) * 10 / 1024;
            int mem_total = Integer.parseInt(st.nextToken()) * 10 / 1024;

            int rx_kbps = Integer.parseInt(st.nextToken()) * 10 * 8 / 1000;
            st.nextToken();
            int tx_kbps = Integer.parseInt(st.nextToken()) * 10 * 8 / 1000;
            st.nextToken();

            graphCPU.addValue( 0, cpu >= 1000 ? 1000 : cpu );
            graphCPU.addValue( 1, mem_used );
            graphCPU.setMaxValue( 1, mem_total );
            graphCPU.addValue( 2, rx_kbps );
            graphCPU.addValue( 3, tx_kbps );
            graphCPU.Refresh();

            if ( dspCount >= 1 )
            {
                 int dsp = Integer.parseInt(st.nextToken());
                 graphDSP.addValue( 0, dsp >= 1000 ? 1000 : dsp );
            }
            
            if ( dspCount >= 2 )
            {
                 int dsp = Integer.parseInt(st.nextToken());
                 graphDSP.addValue( 1, dsp >= 1000 ? 1000 : dsp );
            }
            
            if ( dspCount >= 3 )
            {
                 int dsp = Integer.parseInt(st.nextToken());
                 graphDSP.addValue( 2, dsp >= 1000 ? 1000 : dsp );
            }
            
            if ( dspCount >= 4 )
            {
                 int dsp = Integer.parseInt(st.nextToken());
                 graphDSP.addValue( 3, dsp >= 1000 ? 1000 : dsp );
            }
            
            graphDSP.Refresh();
        }
        catch( NoSuchElementException e ) 
        {
            System.out.println( "Albatross Error no more elements: " +e );
            this.showStatus( "Albatross Error no more elements: " +e );
        }
        catch( NumberFormatException e ) 
        {
            System.out.println( "Albatross Error number format: " +e );
            this.showStatus( "Albatross Error number format: " +e );
        }
        
        //System.out.println( Double.toString( netTX ) +" + " +Double.toString( netRX ) +" = " +Double.toString( netTX+netRX ));
        //System.out.println( portStatus );
        
    }

/***********************************************************/

    public void setPhonePort(int portnr, int status)
    {
        if(portnr < 1 || portnr > iPorts || status < 0 || status > 4 ) 
        {
            System.out.println( "Albatross Error, wrong format in Port status.");
            this.showStatus("Albatross Error, wrong format in Port status.");
            return;
        }
            
        port[portnr-1].loadImage(imgPorts[status]);
    }

/***********************************************************/
    public void openLink(int port)
    {
        if( webPhoneURL == null || webPhoneURL == "" ) 
            return;

        try 
        {
            String phone = Integer.toString( port + 1 );
            URL link = new URL( webPhoneURL + phone );
            getAppletContext().showDocument(link,frame);
            System.out.println( "Opening: " + webPhoneURL + phone );
        }
        catch( NumberFormatException e ) 
        {
            //this.showStatus( "Albatross Error SoftPhone number format: " +e );
        }
        catch( NoSuchElementException e ) 
        {
            System.out.println( "Albatross Error SoftPhone no more elements: " +e );
            this.showStatus( "Albatross Error no SoftPhone more elements: " +e );
        }
        catch(MalformedURLException e) 
        {
            System.out.println("Albatross Error Opening SoftPhone:" +e);
            this.showStatus("Albatross Error Opening SoftPhone:" +e);
        }

    }

/***********************************************************/

}
