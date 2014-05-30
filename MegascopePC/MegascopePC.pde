import processing.serial.*;

Serial port;
int[] data = {-1, 0, 0, 0, 0, 0, 0};
int base_y = 20;
int width_x = 655;
int height_y = 300;

int selected_plot_idx = 0;
int[] plot_base_y = {0, 0, 0, 0, 0, 0, 0};

int last_text_idx = 0;

boolean is_paused = false;
boolean is_port_configured = false;

void viewport()
{
  clear();
  background(200,200,200);
  stroke(0);
  rect((width - width_x)/2, height-base_y, width_x, -height_y, 7);
}

void setup() {
  size(800, 350);
  background(200,200,200); 
  viewport();
}

Serial probe_port(String port_name)
{
  Serial p = new Serial(this, port_name, 38400);
  if (p == null)
    return null;
  
  boolean has_data = false;
  for (int i = 0; i < 10 && !has_data; i++)
  {
    has_data = (p.available() >= 28);
    if (!has_data)
      delay(100);
  }
  
  if (!has_data)
  {
    p.clear();
    p.stop();
    return null;
  }
  
  String s = p.readStringUntil(10);
  if (s != null)
  {
    if (split(s, '\t').length == 7)
      return p;
  }
  
  p.clear();
  p.stop();
  return null;
}

int no_serial_data_counter = 0;

void draw() {

    if (!is_port_configured) {
      String[] ports = Serial.list();
      println(millis(), "Configuring port; available: { ");
      println(ports);
      println("}");
      for (int i = 0; port == null && i < ports.length; i++)
        port = probe_port(ports[i]); 
      
      if (port != null)
      {
        is_port_configured = true;
        viewport();
        println(millis(), "Port OK");
      }
      else
        return;
    }
    
    if (port == null || no_serial_data_counter > 1000)
    {
      is_port_configured = false;
      if (port != null)
      {
        port.clear();
        port.stop();
      }
      println(millis(), "Port lost");
      return;      
    }
    
    no_serial_data_counter ++;
    
    while (port.available() > 0) {
      String s = port.readStringUntil(10);
      if (s == null)
        continue;
      
      if (is_paused)
      {
        data[0] = -1;
        continue;
      }
      
      int[] new_data  =  int(split(s, '\t'));
      if (new_data.length != data.length)
        continue;
        
      no_serial_data_counter = 0;
      
      stroke(0);
      strokeWeight(1);
      for (int i = 1; i < data.length - 1; i++)  {
        println(millis(), new_data[0], data[0]);
        if (new_data[0] < data[0])
        {
          data[0] = 0;
          viewport();
          println(millis(), "wrap");
        }
        if (data[0] < 0)
          data = new_data;
        if (i == selected_plot_idx)
          strokeWeight(2);
        else 
          strokeWeight(1);
        int y0 = height - data[i] - base_y - plot_base_y[i]; 
        int y1 = height - new_data[i] - base_y - plot_base_y[i];
        int Y0 = min(max(y0, height - base_y - height_y), height - base_y);
        int Y1 = min(max(y1, height - base_y - height_y), height - base_y);
        line((width - width_x)/2 + data[0] / 100.0, Y0, (width - width_x)/2 + new_data[0] / 100.0, Y1);
      }      
      data = new_data;
    } 
    
    
    int text_y = height - base_y - height_y - 5;    
    fill(200, 200, 200);
    stroke(200,200,200);
    rect(width / 2 - 10, text_y + 1, 20, -20);
    if (selected_plot_idx >= 1 && selected_plot_idx <=5)
    {
      fill(0, 0, 0);
      text(selected_plot_idx, width / 2, text_y);
      last_text_idx = selected_plot_idx;
    }
    fill(255, 255, 255);     
}

void keyPressed() {
  if (key >= '0' && key <= '5')
    selected_plot_idx = key - '0';
  else if (keyCode == LEFT)
  {
    selected_plot_idx --;
    if (selected_plot_idx < 1)
      selected_plot_idx = 5;
  }
  else if (keyCode == RIGHT)
  {
    selected_plot_idx ++;
    if (selected_plot_idx > 5)
      selected_plot_idx = 1;
  }  
  else if (keyCode == UP)
    plot_base_y[selected_plot_idx] = min(plot_base_y[selected_plot_idx] + 1, height_y);
  else if (keyCode == DOWN)
    plot_base_y[selected_plot_idx] = max(plot_base_y[selected_plot_idx] - 1, 0);
  else if (keyCode == ENTER)
    plot_base_y = new int[]{0, 10, 30, 50, 70, 90, 0};
  else if (key == ' ')
    is_paused = !is_paused;
  
}

