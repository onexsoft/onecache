# onecache

OneCache a Redis protocol based distributed cache middleware, as a replacement of Twemproxy or Codis. Single redis instance is not power enough, so we need groups of redis instances for higher throughput. Unlike twemproxy, OneCache can offer 500,000 or higher QPS for single instance with lower latency. And OneCache can group redis master and slave togather into to one server group for read/write splitting and failover.

Key features of OneCache including :

  1, Multiple threads model provide higher performance for single instance, even without pipeline feature, less proxy instances make operation easy.
  
  2, Server group supported, you can group redis master & slave together for load balance and failover to prevent key remap of the hash range in case of one node failure. Different load balance policies are supported.
  
  3, You move some hot keys to different redis instance in case that there are too much hot keys in a single instance by key based map rules.
  
  4, Unix socket connection enabled.
  
  5, Real time performance statistics, including top keys statistics. 

By the way, we have comercial database proxy for MySQL/PostgreSQL database, which can be a good load balancer for MySQL/PostgreSQL groups, or the SQL sharding routers transparently to you applications, or the SQL fireware to prevent your application from SQL injection hurt. For more information, please visit http://www.mysqlsoft.com

For any question, please contact flou(#)onexsoft.com
