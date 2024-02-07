// common.js

function toggle_inherited(elt)
{
  var table = elt.nextElementSibling;
  if (table.style.display)
  {
    table.style.display = '';
    elt.className = 'inherited-title';
  }
  else
  {
    table.style.display = 'table';
    elt.className = 'inherited-title open';
  }
}
