extern crate resp;

use std::arch::x86_64::_rdtsc;

use resp::encode;
use resp::Data;
use resp::decode;


const RUNS: usize = 500000;

static mut SAMPLES: [u32; RUNS] = [0; RUNS];


fn print(s: [u32; RUNS])
{
  println!("RESP parser\n \
              min: {}\n \
              50%: {}\n \
              99%: {}\n \
              99.9: {}\n \
              max: {}\n",
           s[0],
           s[s.len() / 2],
           s[(s.len() as f32 * 0.99) as usize],
           s[(s.len() as f32 * 0.999) as usize],
           s[s.len() - 1] );
}

fn test( array: &Data )
{
   let encoded = encode(&array);

   //println!("{}", String::from_utf8(encoded.clone()).unwrap());

   let mut i: usize = 0;
   let mut start: u64;
   while i < RUNS
   {
       unsafe { start = _rdtsc() };
       decode(&encoded).ok();
       unsafe { SAMPLES[i] = ( _rdtsc() - start ) as u32 };

       i += 1;
   }

   unsafe {
       SAMPLES.sort();
       print(SAMPLES);
   }

}

fn main() {
   let mut array = Data::Array(vec![Data::BulkString("GET".to_string()),
                                Data::BulkString("Key".to_string())]);

   test(&array);

   array = Data::Array(vec![Data::BulkString("SET".to_string()),
                                Data::BulkString("key".to_string()),
                                Data::BulkString("value".to_string()) ]);
   test(&array);

   array = Data::Array(vec![Data::BulkString("GETRANGE".to_string()),
                                Data::BulkString("key".to_string()),
                                Data::Integer(0),
                                Data::Integer(100) ]);
   test(&array);

}
