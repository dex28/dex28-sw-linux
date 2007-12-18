import java.awt.*;

public class GraphView extends Canvas
{  
    public GraphView(int x, int y,int array,String Name)
    {
        setSize(x, y);

        maxValue = new int[array];
        nrofValues = (x/10)+1;
        values = new int[array][nrofValues];
        color = new Color[array];
        unit = new String[array];
        graphName = Name;

        for(int a = 0; a < array; a++) 
        {
            color[a] = Color.yellow;
            unit[a] = "";
            maxValue[a] = 100;
            for(int i = 0; i < nrofValues; i++)
            {
                values[a][i] = -1;
            }
        }
        
        setBackground(new Color(0,128,32));
    }
/***********************************************************/
    public void paint(Graphics g)
    {  
        Dimension d = getSize();
        Image imgCanvas = createImage( d.width, d.height );
        Graphics graph = imgCanvas.getGraphics();
        int width = d.width;
        int height = d.height-1;
        int exclude = 12;
        int x,y,y2;

        //Draw 3D look
        graph.setColor(new Color(0,64,0));
        graph.fillRect(0,0,width,height);
        graph.setColor(Color.black);
        graph.draw3DRect(0,0,width,height,false);

        graph.setColor(new Color(0,128,0));
        //Draw horisontal green lines
        for(y = height; y > exclude; y -= 10)
            graph.drawLine(1,y,width,y);

        exclude = y+10;
        
        //Draw vertical green lines
        for(x = width-10; x > 2; x -= 10)
            graph.drawLine(x,exclude,x,height);

        //Draw static text
        graph.setColor(Color.yellow);
        Font f = new Font("Helvetica",Font.PLAIN,10);
        graph.setFont(f);
        graph.drawString(graphName,1,10);
        FontMetrics fm = graph.getFontMetrics(f);

        //Draw the curves
        for(int a = 0; a < color.length; a++ ) 
        { 

            graph.setColor(color[a]);

            for(int i = 0; i < nrofValues-1; i++) 
            {
                if( values[a][i] < 0 || values[a][i+1] < 0)
                    break;
                y = height - (((height-exclude)*values[a][i])/maxValue[a]);
                y2 = height - (((height-exclude)*values[a][i+1])/maxValue[a]);
                x = width-(10*i);
                graph.drawLine(x,y,x-10 ,y2);
                //Draw thicker line
                y++;
                y2++;
                graph.drawLine(x,y,x-10 ,y2);
            }

            String str = Integer.toString( values[a][0] / 10 )
                    + "."
                    + Integer.toString( values[a][0] % 10 )
                + unit[a];
                   int textx = fm.stringWidth(str);
                   graph.drawString(str,width-textx,10+a*12);
        }
        graph.dispose();
        g.drawImage(imgCanvas,0,0,this);
    }  

/***********************************************************/
    public void update(Graphics g)
    {  
        paint(g);
    }

/***********************************************************/
    public void Refresh()
    {  
        repaint();
    }

/***********************************************************/
    public void setMaxValue( int array, int v )
    {  
        try 
        {
            maxValue[array] = v;
        }
        catch( ArrayIndexOutOfBoundsException e) 
        {
            System.out.println( "Albatross Error in Array for Color: " +e );
        }
    }
/***********************************************************/
    public void setUnit( int array, String v )
    {  
        try 
        {
            unit[array] = v;
        }
        catch( ArrayIndexOutOfBoundsException e) 
        {
            System.out.println( "Albatross Error in Array for Color: " +e );
        }
    }
/***********************************************************/
    public void setColor(int array, Color col)
    {  
        try 
        {
            color[array] = col;
        }
        catch( ArrayIndexOutOfBoundsException e) 
        {
            System.out.println( "Albatross Error in Array for Color: " +e );
        }
    }
/***********************************************************/
    public void addValue(int array,int value)
    {
        try 
        {  
            for(int i = nrofValues-1; i > 0; i-- )
            {
                values[array][i] = values[array][i-1];
            }
            values[array][0] = value;

            if( maxValue[array] < value )
                maxValue[array] = value;
        }
        catch( ArrayIndexOutOfBoundsException e) 
        {
            System.out.println( "Albatross Error in array for Values: " +e );
        }


    }
    
/***********************************************************/
    
    private int nrofValues;
    private int values[][];
    private int maxValue[];
    private String graphName;
    private Color color[];
    private String unit[];
    //private static final int separation = 10;
}
