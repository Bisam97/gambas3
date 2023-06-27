function $_(a)
{
  return document.getElementById(a);
}

if (!String.prototype.endsWith) 
{
  String.prototype.endsWith = function(searchString, position) 
  {
    var subjectString = this.toString();
    if (typeof position !== 'number' || !isFinite(position) || Math.floor(position) !== position || position > subjectString.length) {
      position = subjectString.length;
    }
    position -= searchString.length;
    var lastIndex = subjectString.indexOf(searchString, position);
    return lastIndex !== -1 && lastIndex === position;
  };
}

Element.prototype.hasClass = function(klass)
{
  if (this.classList)
    return this.classList.contains(klass);
  else
    return !!this.className.match(new RegExp('(\\s|^)' + klass + '(\\s|$)'));
};

Element.prototype.addClass = function(klass)
{
  if (this.classList)
    this.classList.add(klass);
  else if (!this.hasClass(klass))
    this.className += " " + klass;
};

Element.prototype.removeClass = function(klass)
{
  if (this.classList)
    this.classList.remove(klass);
  else if (this.hasClass(klass)) 
  {
    var reg = new RegExp('(\\s|^)' + klass + '(\\s|$)');
    this.className = this.className.replace(reg, ' ');
  }
};

/*Element.prototype.ensureVisible = function()
{
  var parent = this.offsetParent;
  
  while (parent && parent.clientHeight == parent.scrollHeight && parent.clientWidth == parent.scrollWidth)
    parent = parent.offsetParent;
  
  if (parent)
    gw.ensureVisible(this.offsetParent, this.offsetLeft, this.offsetTop, this.offsetWidth, this.offsetHeight);
};*/

gw = {

  version: '0',
  commands: [],
  timers: {},
  windows: [],
  form: '',
  debug: false,
  loaded: {},
  uploads: {},
  autocompletion: [],
  focus: false,
  lock: 0,
  needKeyPress: {},
  drawingContext: [],
  images: {},
  updates: {},
  
  log: function(msg)
  {
    if (gw.debug)
    {
      if (gw.startTime == undefined)
        gw.startTime = Date.now();
      console.log(((Date.now() - gw.startTime) / 1000).toFixed(3) + ': ' + msg);
    }
  },
  
  getFormId: function(id)
  {
    var pos = id.indexOf(".");
    if (pos > 0)
      return id.substr(0, pos);
  },
  
  load: function(lib)
  {
    var elt, src;
    
    if (gw.loaded[lib])
      return;
    
    if (lib.endsWith('.js'))
    {
      elt = document.createElement('script');
      elt.setAttribute("type", "text/javascript");
      src = $root + '/lib:' + lib.slice(0, -3) + ':' + gw.version + '.js';
      elt.setAttribute("src", src);
    }
    else if (lib.endsWith('.css'))
    {
      elt = document.createElement('link');
      elt.setAttribute("rel", "stylesheet");
      elt.setAttribute("type", "text/css");
      src = $root + '/style:' + lib.slice(0, -4) + ':' + gw.version + '.css';
      elt.setAttribute("href", src);
    }
    else
      return;
      
    document.getElementsByTagName("head")[0].appendChild(elt);
    gw.loaded[lib] = src;
    gw.log('load: ' + src);
  },
  
  setInnerHtml: function(id, html)
  {
    var oldDiv = $_(id);
    var newDiv = oldDiv.cloneNode(false);
    newDiv.innerHTML = html;
    oldDiv.parentNode.replaceChild(newDiv, oldDiv);
    gw.onFocus();
  },
  
  setOuterHtml: function(id, html)
  {
    if ($_(id))
    {
      $_(id).outerHTML = html;
      gw.onFocus();
    }
    else
      gw.log('setOuterHtml: ' + id + '? ' + html);
  },
  
  removeElement: function(id)
  {
    var elt = $_(id);
    //for (i = 0; i < id_list.length; i++)
    //{
      //elt = $_(id_list[i]);
      if (!elt)
        return;
    
      //console.log(id + " removed");
    
      elt.parentNode.removeChild(elt);
    //}
  },
  
  insertElement: function(id, parent)
  {
    var elt;
    var pelt = $_(parent);
    
    if (!pelt)
      return;
      
    elt = document.createElement('div');
    elt.id = id;
    pelt.appendChild(elt);
  },
  
  setVisible: function(id, visible)
  {
    var elt = $_(id);
    if (elt)
    {
      if (visible)
        elt.removeClass('gw-hidden');
      else
        elt.addClass('gw-hidden');
    }
  },
  
  saveFocus: function()
  {
    var active = document.activeElement.id;
    var selection;
    
    if (active)
      selection = gw.getSelection($_(active));
    
    return [active, selection];
  },
  
  restoreFocus: function(save)
  {
    var elt;
    
    if (save[0])
    {
      elt = $_(save[0])
      if (elt) 
      {
        elt.focus();
        gw.setSelection(elt, save[1]);
        gw.onFocus();
      }
    }
    //else
    //  gw.active = document.activeElement.id;
  },
  
  wait: function(lock) {
    if (lock)
    {
      if (gw.lock == 0)
      {
        gw.lock_id = setTimeout(function() {
          var elt = $_('gw-lock');
          elt.style.zIndex = 1000;
          elt.style.display = 'block';
          elt.style.opacity = '1';
          gw.lock_id = undefined;
          }, 500);
      }
      
      gw.lock++;
    }
    else
    {
      gw.lock--;
      if (gw.lock == 0)
      {
        if (gw.lock_id)
        {
          clearTimeout(gw.lock_id);
          gw.lock_id = undefined;
        }
        var elt = $_('gw-lock');
        elt.style.display = 'none';
        elt.style.opacity = '0';
      }
    }
  },
  
  answer: function(xhr, after)
  {
    if (xhr.readyState == 4)
    {
      if (xhr.status == 200 && xhr.responseText)
      {
        if (gw.debug && xhr.gw_command)
           gw.log('==> ' + xhr.gw_command + '...');
        
        gw.focus = false;
        var save = gw.saveFocus();
        
        /*if (gw.debug)
          console.log('--> ' + xhr.responseText);*/
        
        /*var r = xhr.responseText.split('\n');
        var i, expr;
        
        for (i = 0; i < r.length; i++)
        {
          expr = r[i].trim();
          if (expr.length == 0)
            continue;
          if (gw.debug)
          {
            if (expr.length > 1024)
              gw.log('--> ' + expr.substr(0, 1024) + '...');
            else
              gw.log('--> ' + expr);
          }
          eval(expr);
        }*/
         
        expr = xhr.responseText;
        if (gw.debug)
        {
          if (expr.length > 1024)
            gw.log('--> ' + expr.substr(0, 1024) + '...');
          else
            gw.log('--> ' + expr);
        }
        try {
          eval(expr);
          }
        catch(e)
        {
          gw.log('eval: ' + expr);
          throw e;
        }
        
        if (!gw.focus)
          gw.restoreFocus(save);
          
        gw.onFocus();
      }
      
      if (after)
        after();
        
      xhr.gw_command && gw.log('==> ' + xhr.gw_command + ' done.');
      
      if (xhr.gw_command && (xhr.gw_command.length < 5 || xhr.gw_command[4] == undefined || xhr.gw_command[4] == false))
        gw.wait(false);
        
      gw.commands.splice(0, 2);
      gw.sendNewCommand();
    }
  },
  
  sendNewCommand: function()
  {
    var command, after, len;
    var xhr;
    
    for(;;) {
    
      len = gw.commands.length;
      
      if (len < 2)
        return;
      
      command = gw.commands[0];
      after = gw.commands[1];
  
      gw.log('[ ' + command + ' ]');
    
      if (command)
      {
        if (command.length < 5 || command[4] == undefined || command[4] == false)
          gw.wait(true);
          
        xhr = new XMLHttpRequest();
        xhr.gw_command = command;
        xhr.open('GET', $root + '/x?c=' + encodeURIComponent(JSON.stringify(command)), true);
        xhr.onreadystatechange = function() { gw.answer(xhr, after); };
        xhr.send(null);
        gw.log("send XMLHttpRequest...");
        return;
      }
      
      if (after)
        after();
      
      gw.commands.splice(0, 2);
    }
  },

  send: function(command, after)
  {
    if (Object.keys(gw.updates).length)
    {
      var updates = gw.updates;
      gw.updates = {};
      
      for (const [key, value] of Object.entries(updates))
        value(key);
    }
  
    gw.log('gw.send: ' + command + ' ' + JSON.stringify(gw.commands));
    
    gw.commands.push(command);
    gw.commands.push(after);
    
    if (gw.commands.length == 2)
      gw.sendNewCommand();
  },

  raise: function(id, event, args, no_wait)
  {
    gw.send(['raise', id, event, args, no_wait]);
  },
  
  update: function(id, prop, value, after)
  {
    gw.send(['update', id, prop, value, true], after);
  },
  
  updateWait: function(id, prop, value, after)
  {
    gw.send(['update', id, prop, value, false], after);
  },
  
  command: function(action)
  {
    gw.send(null, action);
  },

  getSelection: function(o)
  {
    var start, end;
    
    try
    {
      if (o.createTextRange) 
      {
        var r = document.selection.createRange().duplicate();
        r.moveEnd('character', o.value.length)
        if (r.text == '')
          start = o.value.length;
        else
          start = o.value.lastIndexOf(r.text);
        r.moveStart('character', -o.value.length);
        end = r.text.length;
        return [start, end];
      }
      
      if (o.selectionStart && o.selectionEnd)
        return [o.selectionStart, o.selectionEnd];
    }
    catch(e) {};
    
    return undefined;
  },
  
  setSelection: function(o, sel)
  {
    if (sel)
    {
      if (o.setSelectionRange)
        try { o.setSelectionRange(sel[0], sel[1]) } catch(e) {};
    }
  },
  
  onFocus: function()
  {
    var elt = document.activeElement;
    var id, pos, win;
    
    if (gw._focusTimeout)
    {
      clearTimeout(gw._focusTimeout);
      gw._focusTimeout = undefined;
    }
    
    while (elt && (!elt.id || elt.id.indexOf(':') >= 0))
      elt = elt.parentNode;
    
    if (elt && elt.id)
    {
      id = elt.id;
      
      if (gw.window.current && !id.startsWith(gw.window.current + '.'))
      {
        gw.setFocus(gw.active);
        return;
      }
      
      if (gw.active != id)
      {
        gw.active = id;
        gw.update('', '#focus', id);
      }
    }
    else if (gw.active)
    {
      gw.active = undefined;
      gw._focusTimeout = setTimeout(function() 
        {
          gw._focusTimeout = undefined;
          gw.update('', '#focus', '');
        }, 50);
    }
    
    if (elt && elt != document.body)
    {
      pos = gw.getPos(elt);
      $_('gw-focus').style.top = pos.top + 'px';
      $_('gw-focus').style.left = pos.left + 'px';
      $_('gw-focus').style.width = pos.width + 'px';
      $_('gw-focus').style.height = pos.height + 'px';
      win = gw.getWindow(elt);
      $_('gw-focus').style.zIndex = win ? (parseInt(win.style.zIndex) + 2) : 2;
      $_('gw-focus').style.display = 'block';
    }
    else
    {
      $_('gw-focus').style.display = '';
    }
  },
  
  setFocus: function(id)
  {
    var elt = $_(id + ':entry') || $_(id);
    
    if (elt && document.activeElement != elt)
    {
      elt.focus();
      gw.selection = undefined;
      gw.focus = true;
      gw.onFocus();
    }
  },
  
  highlightMandatory: function(id)
  {
    var elt = $_(id);
    var elt_br;
    var div;
    var div_br;
    
    if (elt == undefined || elt.gw_mandatory)
      return;
    
    elt.gw_mandatory = div = document.createElement('div');
    div.className = 'gw-mandatory';
    elt.parentNode.insertBefore(div, elt);
    
    elt_br = gw.getPos(elt);
    div_br = gw.getPos(div);
    
    div.style.top = (elt_br.top - div_br.top) + 'px';
    div.style.left = (elt_br.left - div_br.left) + 'px';
    div.style.width = elt_br.width + 'px';
    div.style.height = elt_br.height + 'px';
  },
  
  addTimer: function(id, delay)
  {
    gw.removeTimer(id);
    gw.timers[id] = setInterval(
      function() {
        if (gw.timers[id + '!'])
          return;
        gw.timers[id + '!'] = true;
        gw.send(['raise', id, 'timer', [], true],
          function() {
            if (gw.timers[id])
              gw.timers[id + '!'] = undefined;
            }
          );
      },
      delay);
  },
  
  removeTimer: function(id)
  {
    var t = gw.timers[id];
    if (t)
    {
      clearInterval(gw.timers[id]);
      gw.timers[id] = undefined;
      gw.timers[id + '!'] = undefined;
    }
  },
  
  getTargetId: function(elt)
  {
    for(;;)
    {
      if (elt.id)
        return elt.id;
      elt = elt.parentNode;
      if (!elt)
        return;
    }
  },

  getPos: function(elt)
  {
    var found, left = 0, top = 0, width = 0, height = 0;
    var offsetBase = gw.offsetBase;
   
    if (!offsetBase && document.body) 
    {
      offsetBase = gw.offsetBase = document.createElement('div');
      offsetBase.style.cssText = 'position:absolute;left:0;top:0';
      document.body.appendChild(offsetBase);
    }
    
    if (elt && elt.ownerDocument === document && 'getBoundingClientRect' in elt && offsetBase) 
    {
      var boundingRect = elt.getBoundingClientRect();
      var baseRect = offsetBase.getBoundingClientRect();
      found = true;
      left = boundingRect.left - baseRect.left; //- document.documentElement.scrollLeft;
      top = boundingRect.top - baseRect.top; //- document.documentElement.scrollTop;
      width = boundingRect.right - boundingRect.left;
      height = boundingRect.bottom - boundingRect.top;
    }
    
    return { found: found, left: left, top: top, width: width, height: height, right: left + width, bottom: top + height };
  },
  
  getWindow: function(elt)
  {
    while (!elt.hasClass('gw-window'))
    {
      elt = elt.parentNode;
      if (elt == document.body || !elt)
        return null;
    }
      
    return elt;
  },

  copy: function(elt)
  {
    navigator.clipboard.writeText(elt.value)
      .then(() => {
        // Success!
      })
      .catch(err => {
        console.log('Unable to copy to the clipboard: ', err);
      });
  },
  /*ensureVisible: function(id, x, y, w, h)
  {
    var elt = typeof(id) == 'string' ? $_(id) : id;
    var pw, ph,cx, cy, cw, ch;
    var xx, yy, ww, hh;
  
    // WW = W / 2
    ww = w / 2;
    //HH = H / 2
    hh = h / 2;
    // XX = X + WW
    xx = x + ww
    // YY = Y + HH
    yy = y + hh;
    
    // PW = Me.ClientW 
    // PH = Me.ClientH
    pw = elt.clientWidth;
    ph = elt.clientHeight;
    
    cx = - elt.scrollLeft;
    cy = - elt.scrollTop;
    cw = elt.scrollWidth;
    ch = elt.scrollHeight;
  
    //If PW < (WW * 2) Then WW = PW / 2
    //If PH < (HH * 2) Then HH = PH / 2
    if (pw < (ww * 2)) ww = pw / 2;
    if (ph < (hh * 2)) hh = ph / 2;
  
    //If CW <= PW Then
    //  WW = 0
    //  CX = 0
    //Endif
    if (cw <= pw) { ww = 0; cx = 0; }
    
    //If CH <= PH Then
    //  HH = 0
    //  CY = 0
    //Endif
    if (ch <= ph) { hh = 0; cy = 0 }
  
    //If XX < (- CX + WW) Then
    //  CX = Ceil(- XX + WW)
    //Else If XX >= (- CX + PW - WW) Then
    //  CX = Floor(- XX + PW - WW)
    //Endif
    if (xx < (- cx + ww))
      cx = - xx + ww;
    else if (xx >= (- cx + pw - ww))
      cx = - xx + pw - ww;
    
    //If YY < (- CY + HH) Then
    //  CY = Ceil(- YY + HH)
    //Else If YY >= (- CY + PH - HH) Then
    //  CY = Floor(- YY + PH - HH)
    //Endif
    
    if (yy < (- cy + hh))
      cy = - yy + hh;
    else if (yy >= (- cy + ph - hh))
      cy = - yy + ph - hh;
  
    //If CX > 0
    //  CX = 0
    //Else If CX < (PW - CW) And If CW > PW Then
    //  CX = PW - CW
    //Endif
    if (cx > 0)
      cx = 0;
    else if (cx < (pw - cw) && cw > pw)
      cx = pw - cw;
  
    //If CY > 0 Then
    //  CY = 0
    //Else If CY < (PH - CH) And If CH > PH Then
    //  CY = PH - CH
    //Endif
    if (cy > 0)
      cy = 0;
    else if (cy < (ph - ch) && ch > ph)
      cy = ph - ch;
  
    //If $hHBar.Value = - CX And If $hVBar.Value = - CY Then Return True
    //Scroll(- CX, - CY)
    elt.scrollLeft = - cx;
    elt.scrollTop = - cy;
  },*/

  window: 
  {
    zIndex: 0,
    current: '',
    
    exist: function(id)
    {
      return gw.windows.indexOf(id) >= 0;
    },
    
    open: function(id, resizable, modal, minw, minh)
    {
      if (!gw.window.exist(id))
      {
        if (gw.windows.length == 0)
        {
          document.addEventListener('mousemove', gw.window.onMove);
          document.addEventListener('mouseup', gw.window.onUp);
          gw.log('document.addEventListener');
        }
        
        gw.windows.push(id);
        
        $_(id).addEventListener('mousedown', gw.window.onMouseDown);
      }
      
      $_(id).gw_resizable = resizable;
      $_(id).gw_modal = modal;
      $_(id).gw_popup = undefined;
      $_(id).gw_transient = undefined;
      
      if (modal)
      {
        $_(id).gw_current = gw.window.current;
        gw.window.current = id.substr(id.lastIndexOf('@'));;
        
        if ($_(id).gw_focus == undefined)
          $_(id).gw_focus = gw.saveFocus();
      } 
      
      if (minw != undefined)
      {
        $_(id).gw_minw = minw;
        $_(id).gw_minh = minh;
      }
      else if ($_(id).gw_minw == undefined)
      {
        $_(id).gw_minw = $_(id).offsetWidth;
        $_(id).gw_minh = $_(id).offsetHeight;
      }
      
      //console.log('gw.window.open: minw = ' + $_(id).gw_minw + ' minh = ' + $_(id).gw_minh);
      
      // Touch events 
      //pane.addEventListener('touchstart', onTouchDown);
      //document.addEventListener('touchmove', onTouchMove);
      //document.addEventListener('touchend', onTouchEnd);
      
      gw.window.refresh();
    },
    
    translate: function(id, x, y)
    {
      $_(id).style.transform = 'translate(' + (x|0) + 'px,' + (y|0) + 'px)';
      gw.onFocus();
    },
    
    popup: function(id, resizable, control, transient, minw, minh)
    {
      var pos;
      
      if (!gw.window.exist(id))
      {
        if (gw.windows.length == 0)
        {
          document.addEventListener('mousemove', gw.window.onMove);
          document.addEventListener('mouseup', gw.window.onUp);
          gw.log('document.addEventListener');
        }
        
        gw.windows.push(id);
        
        $_(id).addEventListener('mousedown', gw.window.onMouseDown);

        pos = gw.getPos($_(control));
        //console.log(pos);
        
        /*$_(id).style.left = pos.left + 'px';
        $_(id).style.top = pos.bottom + 'px';*/
        if (transient)
          gw.window.translate(id, pos.left, pos.top);
        else
          gw.window.translate(id, pos.left, pos.bottom);
      }
      
      $_(id).gw_resizable = resizable;
      $_(id).gw_modal = true;
      $_(id).gw_popup = true;
      $_(id).gw_popup_transient = transient;
      
      if ($_(id).gw_focus == undefined)
        $_(id).gw_focus = gw.saveFocus();
      
      if (minw != undefined)
      {
        $_(id).gw_minw = minw;
        $_(id).gw_minh = minh;
      }
      else if ($_(id).gw_minw == undefined)
      {
        $_(id).gw_minw = $_(id).offsetWidth;
        $_(id).gw_minh = $_(id).offsetHeight;
      }
      
      gw.window.refresh();
      
      $_(id).scrollIntoView({behavior:"auto", block:"nearest"});
    },

    close: function(id)
    {
      var i;
      
      $_(id).removeEventListener('mousedown', gw.window.onMouseDown);
      
      if ($_(id).gw_modal)
        gw.window.current = $_(id).gw_current;
      
      i = gw.windows.indexOf(id);
      if (i >= 0)
      {
        gw.windows.splice(i, 1);
        gw.window.refresh();
      }
      
      if ($_(id).gw_focus)
      {
        gw.restoreFocus($_(id).gw_focus);
        $_(id).gw_focus = undefined;
        gw.onFocus();
      }
      
      $_(id).gw_minw = $_(id).gw_minh = undefined;
    },
    
    refresh: function()
    {
      var i = 0;
      var zi;
      
      while (i < gw.windows.length)
      {
        if ($_(gw.windows[i]))
        {
          zi = 11 + i * 4;
          if ($_(gw.windows[i]).style.zIndex != zi)
            $_(gw.windows[i]).style.zIndex = zi;
          i++;
        }
        else
          gw.windows.splice(i, 1);
      }
      
      gw.window.updateModal();
      
      if (gw.windows.length == 0)
      {
        gw.log('document.removeEventListener');
        document.removeEventListener('mousemove', gw.window.onMove);
        document.removeEventListener('mouseup', gw.window.onUp);
      }
      else
        gw.window.updateTitleBars();
        
      gw.onFocus();
    },
    
    updateTitleBars: function()
    {
      var i, win, last;
      
      for (i = 0; i < gw.windows.length; i++)
      {
        win = gw.windows[i];
        if ($_(win).gw_popup)
          continue;
        $_(win).addClass('gw-deactivated');
        $_(win + '-titlebar').addClass('gw-deactivated');
        last = win;
      }
      
      if (last && !$_(last).gw_popup)
      {
        $_(last).removeClass('gw-deactivated');
        $_(last + '-titlebar').removeClass('gw-deactivated');
      }
    },
    
    raise: function(id, send)
    {
      var i = gw.windows.indexOf(id);
      if (i < 0)
        return;
      
      gw.windows.splice(i, 1);
      gw.windows.push(id);
      
      for (i = 0; i < gw.windows.length; i++)
        $_(gw.windows[i]).style.zIndex = 11 + i * 4;
        
      gw.window.updateTitleBars();
      $_(id).focus();
        
      if (send)
        gw.update('', '#windows', gw.windows);
    },
    
    updateModal: function()
    {
      var i, elt = $_('gw-modal');
      
      for (i = gw.windows.length - 1; i >= 0; i--)
      {
        if ($_(gw.windows[i]).gw_modal)
        {
          gw.window.zIndex = 10 + i * 4;
          elt.style.zIndex = 10 + i * 4;
          elt.style.display = 'block';
          /*if ($_(gw.windows[i]).gw_popup)
            elt.style.opacity = '0';
          else
            elt.style.opacity = '';*/
          gw.onFocus();
          return;
        }
      }
      
      gw.window.zIndex = 0;
      elt.style.display = 'none';
      gw.onFocus();
    },
    
    center: function(id)
    {
      gw.window.translate(id, (window.innerWidth - $_(id).offsetWidth) / 2, (window.innerHeight - $_(id).offsetHeight) / 2);
      gw.window.updateGeometry(id);
    },
    
    isMaximized: function(id)
    {
      return $_(id).gw_save_geometry != undefined;
    },
    
    maximize: function(id)
    {
      var geom = $_(id).gw_save_geometry;
      if (geom != undefined)
      {
        //$_(id).style.left = geom[0];
        //$_(id).style.top = geom[1];
        $_(id).style.transform = geom[0]
        $_(id).style.width = geom[1];
        $_(id).style.height = geom[2];
        $_(id).gw_save_geometry = undefined;
      }
      else
      {
        $_(id).gw_save_geometry = [$_(id).style.transform, $_(id).style.width, $_(id).style.height];
        $_(id).style.transform = '';
        $_(id).style.width = '100%';
        $_(id).style.height = '100%';
      }
      gw.window.updateGeometry(id);
    },
    
    onMouseDown: function(e)
    {
      gw.window.onDown(e);
    },
    
    onDown: function(e)
    {
      var c, win;
      
      gw.window.context = undefined;
      
      if (e.target.className == 'gw-window-button')
        return;
        
      gw.window.onMove(e);
      
      c = gw.window.context;
      if (c == undefined)
        return; 
        
      if ($_(c.id).gw_save_geometry)
        return;
        
      if (c.isMoving || c.isResizing)
      {
        gw.window.raise(c.id);
        gw.window.downEvent = e;
        e.preventDefault();
      }
    },
    
    onDownModal: function(transient)
    {
      var win = gw.windows[gw.windows.length - 1];
      
      if ($_(win).gw_popup)
      {
        if (transient && !$_(win).gw_popup_transient)
          return;
        gw.update(win, '#close');
      }
    },
    
    onMove: function(e) 
    {
      var i, id, elt, b, x, y, bx, by, bw, bh, th;
      var onTopEdge, onLeftEdge, onRightEdge, onBottomEdge, isResizing;
      var MARGINS = 6;
      
      if (gw.window.downEvent)
      {
        gw.window.context.cx = e.clientX;
        gw.window.context.cy = e.clientY;
        gw.window.animate();
        return;
      }
      
      gw.window.context = undefined;
      
      for (i = 0; i < gw.windows.length; i++)
      {
        id = gw.windows[gw.windows.length - i - 1];
        elt = $_(id);
        
        if (elt.style.zIndex < gw.window.zIndex)
          continue;
        
        b = elt.getBoundingClientRect();
        
        bx = b.left; // - MARGINS;
        by = b.top; // - MARGINS;
        bw = b.width; // + MARGINS * 2;
        bh = b.height; // + MARGINS * 2;
        
        x = e.clientX - bx;
        y = e.clientY - by;
        
        //console.log(x + ',' + y + ' : ' + bx + ',' + by + ',' + bw + ',' + bh);
        
        if (x >= 0 && x < bw && y >= 0 && y < bh)
        {
          if (elt.gw_resizable)
          {
            onTopEdge = y < MARGINS;
            onLeftEdge = x < MARGINS;
            onRightEdge = x >= (bw - MARGINS);
            onBottomEdge = y >= (bh - MARGINS);
            
            isResizing = onTopEdge || onLeftEdge || onRightEdge || onBottomEdge;
          }
          else
            onTopEdge = onLeftEdge = onRightEdge = onBottomEdge = isResizing = false;
          
          if ($_(id).gw_popup)
            th = 0;
          else
            th = $_(id + '-titlebar').offsetHeight;
          isMoving = !isResizing && y < (th + MARGINS);
          
          gw.window.context = {
            id: id,
            x: b.left + window.scrollX,
            y: b.top + window.scrollY,
            cx: e.clientX,
            cy: e.clientY,
            w: b.width,
            h: b.height,
            isResizing: isResizing,
            isMoving: isMoving,
            onTopEdge: onTopEdge,
            onLeftEdge: onLeftEdge,
            onRightEdge: onRightEdge,
            onBottomEdge: onBottomEdge
            };
          gw.window.animate();
          break;
        }
      }
    },
    
    updateGeometry: function(id)
    {
      var b = $_(id).getBoundingClientRect();
      gw.update(id, '#geometry', [ b.left + 'px', b.top + 'px', b.width + 'px', b.height + 'px', gw.window.isMaximized(id)]);
      gw.onFocus();
    },
    
    onUp: function(e)
    {
      var c = gw.window.context;
      
      gw.window.downEvent = undefined;
      
      if (c && (c.isMoving || c.isResizing))
      {
        var id = gw.window.context.id;
        gw.window.context = undefined;
        gw.window.raise(id, true);
        gw.window.updateGeometry(id);
      }
    },

    animate: function() 
    {
      var id, elt, c, e, x, y, w, h;
      var minWidth;
      var minHeight;
      
      //requestAnimationFrame(gw.window.animate);
      
      c = gw.window.context;
      if (!c) return;
    
      elt = $_(c.id);
      minWidth = elt.gw_minw;
      minHeight = elt.gw_minh; //$_(c.id + '-titlebar').offsetHeight + 2 + elt.gw_minh;
      e = gw.window.downEvent;
    
      if (c && c.isResizing && e)
      {
        if (c.onRightEdge)
          elt.style.width = Math.max(c.w + c.cx - e.clientX, minWidth) + 'px';
        
        if (c.onBottomEdge)
          elt.style.height = Math.max(c.h + c.cy - e.clientY, minHeight) + 'px';
    
        x = c.x;
        y = c.y;
    
        if (c.onLeftEdge) 
        {
          x = c.x + c.cx - e.clientX;
          w = c.x + c.w - x;
          if (w >= minWidth) 
          {
            elt.style.width = w + 'px';
            //elt.style.left = x + 'px'; 
            gw.window.translate(c.id, x, y);
            //c.x = x;
          }
        }
    
        if (c.onTopEdge) 
        {
          y = c.y + c.cy - e.clientY;
          h = c.y + c.h - y;
          if (h >= minHeight) 
          {
            elt.style.height = h + 'px';
            //elt.style.top = y + 'px'; 
            gw.window.translate(c.id, x, y);
          }
        }
    
        return;
      }
    
      if (c && c.isMoving && e)
      {
        /*elt.style.left = (Math.max(0, c.x + c.cx - e.clientX)) + 'px';
        elt.style.top = (Math.max(0, c.y + c.cy - e.clientY)) + 'px';*/
        gw.window.translate(c.id, Math.max(0, c.x + c.cx - e.clientX), Math.min(window.innerHeight - elt.firstElementChild.offsetHeight, Math.max(0, c.y + c.cy - e.clientY)));
        return;
      }
    
      // This code executes when mouse moves without clicking
    
      if (c.onRightEdge && c.onBottomEdge || c.onLeftEdge && c.onTopEdge)
        elt.style.cursor = 'nwse-resize';
      else if (c.onRightEdge && c.onTopEdge || c.onBottomEdge && c.onLeftEdge)
        elt.style.cursor = 'nesw-resize';
      else if (c.onRightEdge || c.onLeftEdge)
        elt.style.cursor = 'ew-resize';
      else if (c.onBottomEdge || c.onTopEdge)
        elt.style.cursor = 'ns-resize';
      else
        elt.style.cursor = '';
    }
  },
  
  menu:
  {
    hide: function(elt)
    {
      elt.style.display = 'none';
      setTimeout(function() { elt.style.display = ''; }, 150);
    },
    
    click: function(name, event)
    {
      var id = gw.getTargetId(event.target);
      gw.update(name, '#click', id);
      event.stopPropagation();
    }
  },
  
  table:
  {
    selectRange: function(id, start, end, checked)
    {
      var i;
      var tr;
      
      if ($_(id).hasClass('gw-table'))
      {
        if (end < start)
        {
          i = start;
          start = end;
          end = i;
        }
        
        for (i = start; i <= end; i++)
        {
          tr = $_(id + ':' + i);
          if (checked)
            tr.addClass('gw-selected');
          else
            tr.removeClass('gw-selected');
        }
          
        gw.update(id, '!' + start + ':' + end, checked);
      }
      else
      {
        tr = $_(id + ':' + start);
        if (checked)
          tr.addClass('gw-selected');
        else
          tr.removeClass('gw-selected');
        
        gw.update(id, '!' + start, checked);
      }
    },
  
    select: function(id, row, event)
    {
      var elt = $_(id + ':' + row);
      var last = $_(id).gw_current;
      var selected = !elt.hasClass('gw-selected');
      
      if (event)
      {
        if (event.shiftKey && last)
          gw.table.selectRange(id, last, row, selected);
        else
          gw.table.selectRange(id, row, row, selected);
      }
      else
      {
        if (last != undefined)
          $_(id + ':' + last) && $_(id + ':' + last).removeClass('gw-selected');
        elt.addClass('gw-selected');
        gw.update(id, '$' + row, null);
      }
      
      $_(id).gw_current = row;
      
      $_(id).addClass('gw-unselectable');
      setTimeout(function() { $_(id).removeClass('gw-unselectable'); }, 0);
    },
    
    checkRange: function(id, start, end, checked)
    {
      if ($_(id).hasClass('gw-table'))
      {
        if (end < start)
        {
          var i = start;
          start = end;
          end = i;
        }
        
        for (i = start; i <= end; i++)
          $_(id + ':c:' + i).checked = checked;
      
        gw.update(id, '!' + start + ':' + end, checked);
      }
      else
      {
        $_(id + ':c:' + start).checked = checked;
        gw.update(id, '!' + start, checked);
      }
    },
  
    check: function(id, row, event)
    {
      var elt = $_(id + ':c:' + row);
      var checked = !elt.checked;
      var last = $_(id).gw_current;
      var len;
      
      elt.focus();
      
      if (!event && !checked)
        return;
      
      if (event && event.shiftKey && last)
        gw.table.checkRange(id, last, row, checked);
      else
        gw.table.checkRange(id, row, row, checked);
      
      if (event && event.target == elt)
        elt.checked = !checked;

      $_(id).gw_current = row;

      $_(id).addClass('gw-unselectable');
      setTimeout(function() { $_(id).removeClass('gw-unselectable'); }, 0);
    },
    
    ensureVisible: function(id, row)
    {
      var sw = $_(id).firstChild;
      gw.scrollview.scroll(id, sw.scrollLeft, $_(id + ':' + row).offsetTop - sw.clientHeight / 2);
    }
  },
  
  tree:
  {
    expand: function(id, key, open, event)
    {
      gw.update(id, '^' + key, open);
      if (event) event.stopPropagation();
    },
  },
  
  scrollview:
  {
    setHeaders: function(id, hid, vid)
    {
      $_(id).gw_headerh = hid;
      $_(id).gw_headerv = vid;
    },

    ensureVisible: function(id, child)
    {
      var sw = $_(id).firstChild;
      child = $_(child);
      gw.scrollview.scroll(id, child.offsetLeft - (sw.clientWidth - child.offsetWidth) / 2, child.offsetTop - (sw.clientHeight - child.offsetHeight) / 2);
    },
    
    getView: function(elt)
    {
      if (elt.hasClass('gw-listbox'))
        return elt;
      else
        return elt.firstChild;
    },
    
    onScroll: function(id, more, timeout)
    {
      var sw, last;
      var elt = $_(id);
      
      if (!elt)
        return;
      
      sw = gw.scrollview.getView(elt);
      last = elt.gw_last_scroll;
      
      if (last && last[0] == sw.scrollLeft && last[1] == sw.scrollTop)
        return;
      
      elt.gw_last_scroll = [sw.scrollLeft, sw.scrollTop];
      
      gw.log('gw.scrollview.onScroll: ' + id + ' ' + sw.scrollLeft + ',' + sw.scrollTop);
      
      if (more)
      {
        //if ((sw.scrollHeight - sw.scrollTop) === (sw.clientHeight))
        if (sw.scrollTop >= (sw.scrollHeight - sw.clientHeight - 16))
        {
          /*var wait = document.createElement('div');
          wait.className = 'gw-waiting';
          elt.appendChild(wait);*/
          if (elt.gw_scroll)
          {
            clearTimeout(elt.gw_scroll); 
            elt.gw_scroll = undefined;
          }
          
          gw.update(id, '#more', [sw.scrollLeft, sw.scrollTop]);
          return;
        }
      }
      
      if (elt.gw_headerh)
        $_(elt.gw_headerh).firstChild.scrollLeft = sw.scrollLeft;

      if (elt.gw_headerv)
        $_(elt.gw_headerv).firstChild.scrollTop = sw.scrollTop;

      if (elt.gw_noscroll)
      {
        elt.gw_noscroll = undefined;
        return;
      }
      
      if (elt.gw_scroll)
        clearTimeout(elt.gw_scroll); 
      
      elt.gw_scroll = setTimeout(function()
        { 
          var pos = [sw.scrollLeft, sw.scrollTop];
          
          gw.log('gw.control.onScroll (timer): ' + id + ' ' + sw.scrollLeft + ',' + sw.scrollTop);
          clearTimeout(elt.gw_scroll); 
          
          gw.update(elt.id, '#scroll', pos, function() 
            { 
              elt.gw_scroll = undefined; 
              if (pos[0] != sw.scrollLeft || pos[1] != sw.scrollTop)
                gw.scrollview.onScroll(id, more, timeout);
            });
            
          //elt.gw_scroll = undefined;
        }, 250);
    },
    
    scroll: function(id, x, y)
    {
      var sw = gw.scrollview.getView($_(id));
      
      gw.log("gw.control.scroll: " + id + ": " + x + " " + y);
      
      if (x != sw.scrollLeft)
      {
        $_(id).gw_noscroll = true;
        sw.scrollLeft = x;
      }
      if (y != sw.scrollTop)
      {
        $_(id).gw_noscroll = true;
        sw.scrollTop = y;
      }
      if (x != sw.scrollLeft || y != sw.scrollTop)
        gw.update(id, '#scroll', [sw.scrollLeft, sw.scrollTop]); 
    }
  },
  
  file: 
  {
    select: function(id) 
    {
      var elt = $_(id + ':file');
      
      if ($_(id).gw_uploading)
        return;
      
      elt.focus();
      elt.click();
    },
    
    finish: function(xhr)
    {
      if (xhr.gw_progress)
      {
        setTimeout(function() { gw.file.finish(xhr); }, 250);
        return;
      }
      
      gw.update(xhr.gw_id, 'progress', 1, function() {
        gw.answer(xhr); 
        gw.uploads[xhr.gw_id] = undefined;
        //gw.update(xhr.gw_id, 'finish', []);
        xhr.gw_id = undefined;
        });
    },
    
    upload: function(id, key)
    {
      var elt = $_(id + ':file');
      var file = elt.files[0];
      var xhr = new XMLHttpRequest();
      var form = new FormData();
      
      if (gw.uploads[id])
        return;
      
      gw.uploads[id] = xhr;
      
      //gw.log('gw.file.upload: ' + id + ': ' + file.name);
      
      //xhr.gw_progress = 0;
      
      xhr.gw_progress = 1;
      gw.update(id, 'progress', 0, function() { xhr.gw_progress--; });
      
      form.append('file', file);
      form.append('name', file.name);
      form.append('id', id);
      
      //xhr.upload.addEventListener("loadstart", loadStartFunction, false);  
      //xhr.upload.addEventListener("load", transferCompleteFunction, false);
      
      xhr.upload.addEventListener('progress', 
        function(e) 
        {
          //console.log('upload: progress ' + e.loaded + ' / ' + e.total + ' ' + xhr.gw_id + ' ' + e.lengthComputable + ' ' + xhr.gw_progress);
          
          if (xhr.gw_id == undefined)
            return;
            
          if (e.lengthComputable)
          {
            var t = (new Date()).getTime();
            
            if ((xhr.gw_time == undefined || (t - xhr.gw_time) > 50) && xhr.gw_progress == 0)
            {
              xhr.gw_progress++;
              gw.update(xhr.gw_id, 'progress', e.loaded / e.total, function() { xhr.gw_progress--; }); 
              xhr.gw_time = t;
            }
          }
        },
        false);
      
      xhr.gw_command = ['upload', id];
      xhr.gw_id = id;
        
      xhr.open('POST', $root + '/upload:' + key, true);  
      
      xhr.onreadystatechange = function() 
        {
          if (xhr.readyState == 4)
            gw.file.finish(xhr);
        };
        
      xhr.send(form);  
    },
    
    abort: function(id)
    {
      if (gw.uploads[id])
        gw.uploads[id].abort();
    }
  },

  autocomplete: function(id)
  {
    new AutoComplete({
      selector: $_(id + ':entry'),
      cache: false,
      source: function(term, response) {
        var xhr = $_(id).gw_xhr;
        if (xhr)
        {
          try { xhr.abort(); } catch(e) {}
        }
        
        $_(id).gw_xhr = xhr = new XMLHttpRequest();
        
        xhr.open('GET', $root + '/x?c=' + encodeURIComponent(JSON.stringify(['raise', id, 'completion', [term]])), true);
        xhr.onreadystatechange = function() {
          if (xhr.readyState == 4)
          {
            gw.autocompletion = [];
            gw.answer(xhr);
            response(gw.autocompletion);
          }
        };
        xhr.send();
      },
      onSelect: function(e, term, item) {
        gw.textbox.setText(id, gw.textbox.getText(id));
      }
    });
  },
  
  textbox:
  {
    updateText: function(id)
    {
      gw.update(id, 'text', $_(id + ':entry').value);
    },
    
    onActivate: function(id, e)
    {
      if (e.keyCode == 13)
        setTimeout(function() { gw.textbox.updateText(id); gw.raise(id, 'activate', [], false); }, 20);
    },
    
    onUpdate: function(id)
    {
      gw.updates[id] = gw.textbox.updateText;
    },
    
    onChange: function(id)
    {
      if ($_(id).gw_timer) clearTimeout($_(id).gw_timer);
      $_(id).gw_timer = setTimeout(function() { gw.textbox.updateText(id); }, 20);
    },
    
    getText: function(id)
    {
      return $_(id + ':entry').value;
    },
    
    moveEnd: function(id)
    {
      gw.setSelection($_(id + ':entry'), [-1, -1]);
    },
    
    setText: function(id, text)
    {
      gw.command(function() {
        $_(id + ':entry').value = text;
        gw.textbox.moveEnd(id);
        gw.update(id, 'text', text);
        });
    },
    
    clear: function(id)
    {
      gw.textbox.setText(id, '');
      gw.setFocus(id);
      gw.raise(id, 'activate', [], false);
    },
    
    copy: function(id)
    {
      gw.copy($_(id + ':entry'));
    }
  },

  textarea:
  {
    updateText: function(id)
    {
      gw.update(id, 'text', $_(id).value);
    },
    
    onUpdate: function(id)
    {
      gw.updates[id] = gw.textarea.updateText;
    },
    
    onChange: function(id)
    {
      if ($_(id).gw_timer) clearTimeout($_(id).gw_timer);
      $_(id).gw_timer = setTimeout(function() { gw.textarea.updateText(id); }, 20);
    },
    
    copy: function(id)
    {
      gw.copy($_(id));
    },
    
    moveEnd: function(id)
    {
      gw.setSelection($_(id), [-1, -1]);
      $_(id).scrollTop = $_(id).scrollHeight;
    },
    
    setText: function(id, text)
    {
      gw.command(function() {
        $_(id).value = text;
        gw.textarea.moveEnd(id);
        gw.update(id, 'text', text);
        });
    },
  },
  
  button:
  {
    setText: function(id, text)
    {
      $_(id).lastElementChild.innerHTML = text;
    },
    
    click: function(id)
    {
      $_(id).focus();
      $_(id).click();
    }
  },
  
  combobox:
  {
    resize: function(id)
    {
      $_(id + ':select').onmouseover = function() { $_(id + ':select').style.width = $_(id).offsetWidth + 'px'; }
    },
    
    update: function(id, index, text)
    {
      if (text != undefined)
        $_(id + ':entry').value = text;
        
      $_(id + ':select').selectedIndex = index;
    }
  },
  
  listbox:
  {
    selectRange: function(id, start, end, checked)
    {
      var items = $_(id).children;
      var i;
      var elt;
      
      if (end < start)
      {
        i = start;
        start = end;
        end = i;
      }
      
      for (i = start; i <= end; i++)
      {
        elt = items[i];
        if (checked)
          elt.addClass('gw-selected');
        else
          elt.removeClass('gw-selected');
      }
        
      gw.update(id, checked ? '+' : '-',  [start, end - start + 1]);
    },
  
    select: function(id, row, event, multiple)
    {
      var items = $_(id).children;
      var last = $_(id).gw_current;
      var elt = items[row];
      var selected;
      
      if (event.detail == 2 || !elt)
        return;
      
      selected = !elt.hasClass('gw-selected');
      
      if (multiple)
      {
        if (event.shiftKey && last)
          gw.listbox.selectRange(id, last, row, selected);
        else
          gw.listbox.selectRange(id, row, row, selected);
      }
      else
      {
        if (last != undefined)
          items[last] && items[last].removeClass('gw-selected');
        elt.addClass('gw-selected');
        gw.listbox.ensureVisible(id, row);
        gw.update(id, '$', row);
      }
      
      $_(id).gw_current = row;
      
      /*$_(id).addClass('gw-unselectable');
      setTimeout(function() { $_(id).removeClass('gw-unselectable'); }, 0);*/
    },
    
    onKeyDown: function(id, event)
    {
      var row;
      
      if (event.key == 'ArrowDown')
      {
        if (!(event.altKey || event.shiftKey || event.ctrlKey))
        {
          row = $_(id).gw_current;
          gw.listbox.select(id, row + 1, event, false);
          event.preventDefault();
        }
      }
      else if (event.key == 'ArrowUp')
      {
        if (!(event.altKey || event.shiftKey || event.ctrlKey))
        {
          row = $_(id).gw_current;
          gw.listbox.select(id, row - 1, event, false);
          event.preventDefault();
        }
      }
    },
    
    ensureVisible: function(id, row)
    {
      var elt = $_(id);
      var child = elt.children[row];
      
      //console.log(row + ' / ' + (child.offsetTop + child.offsetHeight) + ' / ' + elt.scrollTop + ' / ' + elt.clientHeight);
      
      if ((child.offsetTop + child.offsetHeight) >= (elt.scrollTop + elt.clientHeight))
        gw.scrollview.scroll(id, elt.scrollLeft, child.offsetTop + child.offsetHeight - elt.clientHeight);
      else if (child.offsetTop < elt.scrollTop)
        gw.scrollview.scroll(id, elt.scrollLeft, child.offsetTop);
    }
  },
  
  image:
  {
    preload: function(images)
    {
      var image;
      var i;
      
      for (i = 0; i < images.length; i++)
      {
        image = new Image();
        image.src = images[i];
      }
    }
  },
  
  drawingarea:
  {
    init: function(id)
    {
      if (!gw.resizeObserver)
      {
        gw.resizeObserver = new ResizeObserver(function(entries)
          {
            for (let elt of entries)
              gw.drawingarea.update(elt.target.id);
          });
      }

      gw.resizeObserver.observe($_(id));
      gw.drawingarea.update(id);
    },
    
    update: function(id)
    {
      var w = $_(id + ':canvas').offsetWidth;
      var h = $_(id + ':canvas').offsetHeight;
      if (w != $_(id + ':canvas').width || h != $_(id + ':canvas').height)
      {
        $_(id + ':canvas').width = w;
        $_(id + ':canvas').height = h;
      }
      gw.update(id, '#', [w, h]);
    },
    
    onMouseDown: function(event, id, down, move, up)
    {
      window.addEventListener('mouseup', gw.drawingarea.onMouseUp);
      
      if (move)
        window.addEventListener('mousemove', gw.drawingarea.onMouseMove);
      
      gw.grab = {
        'id': id,
        'move': move,
        'up': up,
        'dx': event.screenX - event.offsetX,
        'dy': event.screenY - event.offsetY,
        'sx': event.offsetX,
        'sy': event.offsetY
        };
        
      if (down)
        gw.sendMouseEvent(event, 'MouseDown');
    },
    
    onMouseUp: function(event)
    {
      if (gw.grab.up) gw.sendMouseEvent(event, 'MouseUp');
      if (gw.grab.move) window.removeEventListener('mousemove', gw.drawingarea.onMouseMove);
      window.removeEventListener('mouseup', gw.drawingarea.onMouseUp);
      gw.grab = undefined;
    },
    
    sendMouseMoveEvent: function()
    {
      gw.sendMouseEvent(gw.mouseMove, 'MouseMove', function() { gw.mouseMove = undefined; })
    },
    
    onMouseMove: function(event)
    {
      var send = gw.mouseMove == undefined;
      gw.mouseMove = event;
      if (send)
        gw.drawingarea.sendMouseMoveEvent();
    }
  },
  
  paint:
  {
    makeGradient: function(ctx, mo, coords, stops)
    {
      var grad, i, st;
      
      if (mo == -1)
        return;
        
      if (mo == 0)
        grad = ctx.createLinearGradient(coords[0], coords[1], coords[2], coords[3]);
      else
        grad = ctx.createRadialGradient(coords[0], coords[1], coords[2], coords[3], coords[4], coords[5]);
      
      for (i = 0; i < stops.length; i++)
      {
        st = stops[i];
        grad.addColorStop(st[0] ,st[1]);
      }

      return grad;
    },
    
    loadImage: function(url, func)
    {
      var img = gw.images[url];
      
      if (img)
      {
        func(img);
      }
      else
      {
        img = new Image();
        img.onload = function() { func(img); gw.images[url] = img; }
        img.src = url;
      }
    }
  },
  
  sound:
  {
    pause: function(id)
    {
      var elt = $_(id);
      elt.pause();
    },
    
    stop: function(id)
    {
      var elt = $_(id);
      elt.pause();
      elt.currentTime = 0;
    },
    
    play: function(id)
    {
      var elt = $_(id);
      if (!elt.paused) 
      {
        elt.pause();
        elt.currentTime = 0;
      }
      elt.play();
    }
  },
  
  makeShortcut: function(event)
  {
    var shortcut;
    
    if (event.key == undefined)
      return;
    
    shortcut = '';
    if (event.ctrlKey && event.key != 'Control') shortcut += 'CTRL+';
    if (event.shiftKey && event.key != 'Shift') shortcut += 'SHIFT+';
    if (event.altKey && event.key != 'Alt') shortcut += 'ALT+';
    if (event.meta && event.key != 'Meta') shortcut += 'META+';
    shortcut += event.key.toUpperCase();
    return shortcut;
  },
  
  sendKeyPress: function(event, id)
  {
    gw.send(['keypress', id, 
      {
        'altKey': event.altKey,
        'ctrlKey': event.ctrlKey,
        'key': event.key,
        'metaKey': event.meta,
        'shiftKey': event.shiftKey
      }],
      null);
  },
  
  sendMouseEvent: function(event, type, func)
  {
    gw.send(['mouse', gw.grab.id, type, 
      {
        'x': event.screenX - gw.grab.dx,
        'y': event.screenY - gw.grab.dy,
        'sx': event.screenX,
        'sy': event.screenY,
        'button': event.button,
        'buttons': event.buttons,
        'altKey': event.altKey,
        'ctrlKey': event.ctrlKey,
        'metaKey': event.meta,
        'shiftKey': event.shiftKey
      }],
      func);
  },
  
  raise: function(id, event, args, no_wait)
  {
    gw.send(['raise', id, event, args, no_wait]);
  },

  onKeyDown: function(event)
  {
    var elt;
    var id;
    
    if (event.bubbles && gw.shortcuts)
    {
      var shortcut = gw.makeShortcut(event);
      if (shortcut && gw.shortcuts[shortcut])
      {
        //gw.log('shortcut -> ' + shortcut);
        gw.sendKeyPress(event, '');
        event.preventDefault();
        return;
      }
    }
    
    if (!event.bubbles)
      return;
    
    id = '';
    elt = document.activeElement;
    while (elt) // TODO: stop at window?
    {
      id = elt.id;
      if (id && id.indexOf(':') < 0)
        break;
      elt = elt.parentNode;
    }
  
    if (id && (gw.needKeyPress[id] != undefined || gw.needKeyPress[gw.getFormId(id)]))
      gw.sendKeyPress(event, id);
  },
  
  body:
  {
    onLoad: function()
    {
      document.body.addEventListener('focusin', gw.onFocus);
      document.body.addEventListener('focusout', gw.onFocus);
      gw.raise(null, 'open');
      document.body.style.opacity = '1';
      document.body.style.pointerEvents = 'auto';
    },
    
    onResize: function()
    {
      gw.onFocus();
    }
  }
  
}

document.onkeydown = gw.onKeyDown;
