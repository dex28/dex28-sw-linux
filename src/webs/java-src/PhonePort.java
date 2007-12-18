import java.awt.*;
import java.awt.event.MouseEvent;

public class PhonePort extends Canvas
{  
    public PhonePort(Image image, int x, int y, int port, Albatross parent)
    {  
        img = image;
        setSize(x, y);
        portnr = port;
        albatross = parent;
        enableEvents (AWTEvent.MOUSE_EVENT_MASK);
    }

/***********************************************************/
    public void paint(Graphics g)
    {  
        if (img != null)
            g.drawImage(img, 0, 0, Color.black, this);
    }  
    
/***********************************************************/
    public void update(Graphics g)
    {  
        paint(g);
    }
/***********************************************************/
    protected void processMouseEvent( MouseEvent e )
    {
        if( e.getID () == MouseEvent.MOUSE_PRESSED ) 
        {
            albatross.openLink(portnr);
        }
        super.processMouseEvent( e );
    }

/***********************************************************/
    public void loadImage(Image Pic)
    {  
        if( Pic == img )
            return;
        img = Pic;
        repaint();
    }

/***********************************************************/

    private Image img;
    private int portnr;
    private Albatross albatross;
}
