{
  'targets': [
    {
      'target_name': 'tripwire',
      'sources': [ 
      	'src/tripwire.cc'
      ],
      'conditions': [
      	['OS=="win"', {
      	  'sources+': [
      	    'src/tripwire_win.cc'
      	  ]
      	}],
      	['OS=="mac"', {
      	  'sources+': [
      	    'src/tripwire_mac.cc'
      	  ]
      	}]
      ]
    }
  ]
}