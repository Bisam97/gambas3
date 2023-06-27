var goto_line_timer;

function $(id)
{
  return document.getElementById(id);
}

function do_goto_line(line)
{
  var sel = [null, null, null];
  var r1, r2;
  var i, n;
  var elt = $('l' + line);
  var elt_next;
  
  for (i = 0; i < sel.length; i++)
  {
    sel[i] = $('_sel' + i);
    if (!sel[i])
    {
      sel[i] = document.createElement('div');
      sel[i].id = '_sel' + i;
      sel[i].style.position = 'absolute';
      sel[i].style.backgroundColor = '#FFFF80';
      sel[i].style.opacity = '0.3';
      sel[i].style.pointerEvents = 'none';
      sel[i].style.zIndex = 1000;
      document.body.appendChild(sel[i]);
    }
    sel[i].style.display = 'none';
  }
  
  if (!elt || elt == undefined)
  {
    document.body.scrollTop = 0;
    return;
  }
  
  r1 = elt.getBoundingClientRect();
  
  r1.x = r1.left + document.body.scrollLeft;
  r1.y = r1.top + document.body.scrollTop;
  r1.width = r1.right - r1.left;
  r1.height = r1.bottom - r1.top;
  
  for (n = 1;; n++)
  {
    elt_next = $('l' + (line+1));
    if (elt_next)
    {
      r2 = elt_next.getBoundingClientRect();
      r2.x = r2.left + document.body.scrollLeft;
      r2.y = r2.top + document.body.scrollTop;
      r2.width = r2.right - r2.left;
      r2.height = r2.bottom - r2.top;
    }
    else
      return;
    /*else
      r2 = {x: 0, y: document.body.offsetHeight, width: document.body.offsetWidth, height: 0};*/
    
    /*if (!elt_next || r2.y > r1.y || r2.x != r1.x)*/
      break;
  }
  
  //alert(line + ': ' + r1.x + ' ' + r1.y + ' / ' + (line + n) + ': ' + r2.x + ' ' + r2.y);
  
  sel[0].style.left = r1.x + 'px';
  sel[0].style.top = r1.y + 'px';
  sel[0].style.right = '0';
  sel[0].style.height = r1.height + 'px';
  sel[0].style.display = 'block';
  
  sel[1].style.left = '0';
  sel[1].style.top = (r1.y + r1.height) + 'px';
  sel[1].style.right = '0';
  sel[1].style.height = Math.max(0, (r2.y - r1.y - r1.height)) + 'px';
  sel[1].style.display = 'block';
  
  sel[2].style.left = '0';
  sel[2].style.top = r2.y + 'px';
  sel[2].style.width = (r2.x + r2.width) + 'px';
  sel[2].style.height = r2.height + 'px';
  sel[2].style.display = 'block';
  
  //elt.scrollIntoView({block:'center'});
  //alert(document.body.scrollTop + ' / ' + ((r1.y + r2.y) / 2) + ' / ' + (document.body.offsetHeight/2));
  document.body.scrollTop = (r1.y + r2.y) / 2 - window.innerHeight / 2;
}

function goto_line(line)
{
  if (goto_line_timer)
    clearTimeout(goto_line_timer);
  goto_line_timer = setTimeout(function() { do_goto_line(line); goto_line_timer = undefined;}, 100);
}