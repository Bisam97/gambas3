function highlight_text(text, search)
{
  var newText = '';
  var i = -1;
  var lcSearch = search.toLowerCase();
  var lcText = text.toLowerCase();

  while (text.length > 0)
  {
    i = lcText.indexOf(lcSearch, i+1);
    if (i < 0)
    {
      newText += text;
      text = '';
    }
    else
    {
      // skip anything inside an HTML tag
      if (text.lastIndexOf('>', i) >= text.lastIndexOf('<', i))
      {
        // skip anything inside a <script> block
        if (lcText.lastIndexOf('/script', i) >= lcText.lastIndexOf('<script', i))
        {
          newText += text.substring(0, i) + '<mark>' + text.substr(i, search.length) + '</mark>';
          text = text.substr(i + search.length);
          lcText = text.toLowerCase();
          i = -1;
        }
      }
    }
  }
  
  return newText;
}

function highlight_body(searches)
{
  var text = document.body.innerHTML;
  
  for (var i = 0; i < searches.length; i++)
    text = highlight_text(text, searches[i]);
    
  document.body.innerHTML = text;
}

function highlight_check()
{
  var i, pair;
  var query = window.location.search.substring(1).split('&');
  
  for (i = 0; i < query.length; i++)
  {
    pair = query[i].split('=');
    if (pair[0] == 'ht' && pair.length == 2)
    {
      highlight_body(decodeURIComponent(pair[1].replace(/[+]/g, '%20')).split(','));
      break;
    }
  }
}
