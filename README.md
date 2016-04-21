# onecache

OneCache a Redis protocol based distributed cache middleware, as a replacement of Twemproxy or Codis. Single redis instance is not power enough, so we need groups of redis instances for higher throughput. Unlike twemproxy, OneCache can offer 500,000 or higher QPS for single instance with lower latency. And OneCache can group redis master and slave together into one server group for read/write splitting and failover.

Key features of OneCache including :

  1, Multiple threads model provide higher performance for single instance, even without pipeline feature, less proxy instances make operation easy.
  
  2, Server group supported, you can group redis master & slave together for load balance and failover to prevent key remap of the hash range in case of one node failure. Different load balance policies are supported.
  
  3, You move some hot keys to different redis instance in case that there are too much hot keys in a single instance by key based map rules.
  
  4, Cross node operation support including "mget", "mset" etc.
  
  5, Unix socket connection enabled.
  
  6, Real time performance statistics, including top keys statistics. 
  
  7, Daemon process protect you from proxy instance failure.
  
  8, Virtual IP based HA feature protect you from proxy host failure.

By the way, we have comercial database proxy for MySQL/PostgreSQL database, which can be a good load balancer for MySQL/PostgreSQL groups, or the SQL sharding routers transparently to you applications, or the SQL fireware to prevent your application from SQL injection hurt. For more information, please visit http://www.mysqlsoft.com

For any question, please contact by email flou(#)onexsoft.com, or chat by QQ 37223884

# Update 2016/04/21

  1, Add client side pipeline support
  
  2, Add twemproxy compatible mode (exactly the same consistent hash logic) to work togather with Twemproxy.
  
  3, Add some debug log information.

# User List

http://daojia.com
