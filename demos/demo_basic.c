/*
 * demo_basic.c: a simple tuibox.h demo
 */

#include <math.h>

#include "../tuibox.h"

/* Global UI struct */
ui_t u;

/* Functions that generate box contents */
void text(ui_box_t *b, char *out){
  sprintf(out, "%s", (char*)b->data1);
}

void draw(ui_box_t *b, char *out){
  int x, y, len = 0;
  const int max_len = MAXCACHESIZE - 100; /* Leave safety buffer */

  out[0] = '\0';
  for(y=0; y<b->h && len < max_len; y++){
    for(x=0; x<b->w && len < max_len; x++){
      /* Truecolor string to generate gradient */
      int written = sprintf(out + len, "\x1b[48;2;%i;%i;%im ", 
        (int)round(255 * ((double)x / (double)b->w)), 
        (int)round(255 * ((double)y / (double)b->h)), 
        (int)round(255 * ((double)x * (double)y / ((double)b->w * (double)b->h))));
      
      if (len + written >= max_len) break;
      len += written;
    }
    if (len + 10 < max_len) { /* Check space for reset sequence */
      strcpy(out + len, "\x1b[0m\n");
      len += 5; /* Length of "\x1b[0m\n" */
    } else {
      break; /* No more space */
    }
  }
  
  /* Ensure null termination */
  out[len] = '\0';
}

/* Function that runs on box click */
void click(ui_box_t *b, int x, int y){
  b->data1 = "\x1b[0m                \n  you clicked me!  \n                ",
  ui_draw_one(b, 1, &u);
}

/* Function that runs on box hover */
void hover(ui_box_t *b, int x, int y, int down){
  b->data1 = "\x1b[0m                \n  you hovered me!  \n                ",
  ui_draw_one(b, 1, &u);
}

void stop(){
  ui_free(&u);
  exit(0);
}

int main(){
  /* Initialize UI data structures */
  ui_new(0, &u);

  /* Add new UI elements to screen 0 - limit size to prevent buffer overflow */
  ui_add(
    1, 1,
    (u.ws.ws_col > 80) ? 80 : u.ws.ws_col, 
    (u.ws.ws_row > 24) ? 24 : u.ws.ws_row,
    0,
    NULL, 0,
    draw,
    NULL,
    NULL,
    NULL,
    NULL,
    &u
  );

  ui_text(
    ui_center_x(19, &u), UI_CENTER_Y,
    "\x1b[0m                   \n    click on me!   \n                   ",
    0,
    click, hover,
    &u
  );

  /* Register an event on the q key */
  ui_key("q", stop, &u);

  /* Render the screen */
  ui_draw(&u);

  ui_loop(&u){
    /* Check for mouse/keyboard events */
    ui_update(&u);
  }

  /* This should never be reached due to ui_key("q", stop, &u), but just in case */
  ui_free(&u);
  return 0;
}
