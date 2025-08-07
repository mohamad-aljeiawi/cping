MY VPS

PROJECT CPING VPS ================
--supabase
API URL: https://supabase.api.cping.nuzomix.com
GraphQL URL: https://supabase.api.cping.nuzomix.com/graphql/v1
S3 Storage URL: https://supabase.api.cping.nuzomix.com/storage/v1/s3
DB URL: postgresql://postgres:postgres@127.0.0.1:54322/postgres
Studio URL: https://supabase.studio.cping.nuzomix.com
Inbucket URL: http://127.0.0.1:54324
JWT secret: ebb56de2c4b4eb4d14b81f5d66eb50c3555d84a47127e8511cb024888e162969
anon key: eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZS1kZW1vIiwicm9sZSI6ImFub24iLCJleHAiOjE5ODM4MTI5OTZ9.tymnzlYM_oh8uI1JZcsaA2Dgm7fddbjCIHhQIA653UA
service_role key: eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZS1kZW1vIiwicm9sZSI6InNlcnZpY2Vfcm9sZSIsImV4cCI6MTk4MzgxMjk5Nn0.TS-hRG1rI8xYmttAuMhgPdiYZYY3Hp--dKs3dnETqsg
S3 Access Key: 625729a08b95bf1b7ff351a663f3a23c
S3 Secret Key: 850181e4652dd023b7a98c58ae0d2d34bd487ee0cc3254aed6eda37307425907
S3 Region: local


--nginx
sudo nano /etc/nginx/sites-available/supabase-api-cping
"
server {
    listen 80;
    listen [::]:80;

    server_name supabase.api.cping.nuzomix.com;

    location / {
        proxy_pass https://127.0.0.1:54321; # IP address and port where your application is running
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;

        # Additional settings if your application uses WebSockets
        proxy_https_version 1.1;
        proxy_set_header Upgrade $https_upgrade;
        proxy_set_header Connection "upgrade";
    }
}
"
sudo nano /etc/nginx/sites-available/supabase-studio-cping
"
server {
    listen 80;
    listen [::]:80;

    server_name supabase.studio.cping.nuzomix.com;

    location / {
        proxy_pass https://127.0.0.1:54323; # IP address and port where your application is running
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;

        # Additional settings if your application uses WebSockets
        proxy_https_version 1.1;
        proxy_set_header Upgrade $https_upgrade;
        proxy_set_header Connection "upgrade";
		
		auth_basic "Restricted Area";
        auth_basic_user_file /etc/nginx/.htpasswd;
    }
}
"
sudo ln -s /etc/nginx/sites-available/supabase-api-cping /etc/nginx/sites-enabled/
sudo ln -s /etc/nginx/sites-available/supabase-studio-cping /etc/nginx/sites-enabled/
sudo systemctl reload nginx

--httpsS certbot
sudo certbot --nginx -d supabase.api.cping.nuzomix.com -d supabase.studio.cping.nuzomix.com

--base auth any domin
echo "hO+OgBtk0tcRHiVJRJpHfXA8" | sudo htpasswd -c -i /etc/nginx/.htpasswd "supabase_cping" --cping auth 
-add to nginx
auth_basic "Restricted Area";
auth_basic_user_file /etc/nginx/.htpasswd;

--generation pass
openssl rand -base64 12

sudo apt install apache2-utils -y

cping.nuzomix.com







create or replace function fun_verify_otp (otp_input text, telegram_id_input bigint) returns table (user_id text, token text) language plpgsql security definer as $$
declare
  uid bigint;
  role_text text;
  otp_expires_at timestamp;
begin
  -- Trying to get the person associated with the OTP
  set search_path = public;
  
  select
    p.id,
    o.expires_at
  into
    uid,
    otp_expires_at
  from otps o
  join persons p on o.person_id = p.id
  where o.otp = otp_input
    and p.telegram_id = telegram_id_input
  order by o.created_at desc
  limit 1;

  -- Check if the code exists
  if uid is null then
    raise exception 'The verification code is invalid or does not belong to this account.';
  end if;

  -- Check code expiration
  if otp_expires_at < now() then
    raise exception 'The verification code has expired.';
  end if;

  --Fetch role (or use "user" as default)
  select role into role_text
  from person_roles
  where person_id = uid
  limit 1;

  if role_text is null then
    role_text := 'user';
  end if;

  -- Generate token using pgjwt
return query
select
  uid::text as user_id,
  extensions.sign(
    json_build_object(
      'sub', uid::text,
      'telegram_id', telegram_id_input::text,
      'role', 'authenticated',
      'exp', extract(epoch from now() + interval '24 hours')::int
    ),
    'ebb56de2c4b4eb4d14b81f5d66eb50c3555d84a47127e8511cb024888e162969',
    'HS256'
  ) as token;
end;
$$;

select
  extensions.sign(
    json_build_object(
      'sub','8',
      'telegram_id', '871050898',
      'role', 'authenticated',
      'exp', extract(epoch from now() + interval '24 hours')::int
    ),
    'ebb56de2c4b4eb4d14b81f5d66eb50c3555d84a47127e8511cb024888e162969',
    'HS256'
  );

select
  extensions.verify(
    'eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiIDogIjEiLCAidGVsZWdyYW1faWQiIDogIjg3MTA1MDg5OCIsICJyb2xlIiA6ICJ1c2VyIiwgImV4cCIgOiAxNzQ3ODQxMzMxfQ.fxwFouL4iPkpvS0A41WUvKx53LusbmUZ6ucB5CTvSfc',
    'ebb56de2c4b4eb4d14b81f5d66eb50c3555d84a47127e8511cb024888e162969',
    'HS256'
  );

select
  *
from
  fun_verify_otp ('E49YO9', 871050898)
select
  n.nspname as schema_name,
  p.proname,
  pg_get_function_identity_arguments(p.oid) as args
from
  pg_proc p
  join pg_namespace n on p.pronamespace = n.oid
where
  p.proname = 'sign';




select * from persons where telegram_id = '871050898';

drop policy if exists select_persons on persons;

create policy select_persons on persons
for select
using (
  telegram_id::text = (current_setting('request.jwt.claims', true)::json ->> 'telegram_id')
  or is_admin()
);



set local request.jwt.claims = '{
  "sub": "8",
  "telegram_id": "871050898",
  "role": "user"
}';

select * from persons;


create or replace function debug_jwt_claims()
returns json as $$
begin
  return current_setting('request.jwt.claims', true)::json;
end;
$$ language plpgsql stable;













 if (strstr(class_name.c_str(), "BP_Character") || strstr(class_name.c_str(), "BP_Player"))
            {
                if (shared_response.Count >= MAX_OBJECTS)
                    break;

                // Class: STExtraBaseCharacter.STExtraCharacter.UAECharacter.Character.Pawn.Actor.Object
                // // CharacterWeaponManagerComponent* WeaponManagerComponent;//[Offset: 0x2488, Size: 0x8]
                uintptr_t weapon_manager = Memory::Read<uintptr_t>(actor + 0x2488, target_pid);
                if (!weapon_manager)
                    continue;

                // Class: WeaponManagerComponent.ActorComponent.Object
                // // STExtraWeapon* CurrentWeaponReplicated;//[Offset: 0x558, Size: 0x8]
                uintptr_t current_weapon = Memory::Read<uintptr_t>(weapon_manager + 0x558, target_pid);

                // Class: STExtraWeapon.LuaActor.Actor.Object
                // // WeaponEntity* WeaponEntityComp;//[Offset: 0x770, Size: 0x8]
                uintptr_t weapon_entity = Memory::Read<uintptr_t>(current_weapon + 0x770, target_pid);

                // Class: WeaponEntity.WeaponLogicBaseComponent.ActorComponent.Object
                // // int WeaponId;//[Offset: 0x178, Size: 0x4]
                int weapon_id = Memory::Read<int>(weapon_entity + 0x178, target_pid);

                // if (weapon_id == 0)
                //     continue;

                // Class: Character.Pawn.Actor.Object
                // // SkeletalMeshComponent* Mesh;//[Offset: 0x498, Size: 0x8]
                uintptr_t skeletal_mesh_component = Memory::Read<uintptr_t>(actor + 0x498, target_pid);
                if (!skeletal_mesh_component)
                    continue;

                // Class: BoxSphereBounds
                Structs::FBoxSphereBounds bounds = Memory::Read<Structs::FBoxSphereBounds>(skeletal_mesh_component + 0x9D4, target_pid);
                Structs::FVector origin = bounds.Origin;
                Structs::FVector extent = bounds.BoxExtent;
                Structs::FTransform transform = Memory::Read<Structs::FTransform>(skeletal_mesh_component + Offset::component_to_world, target_pid);

                extent.X *= 0.45f;
                extent.Y *= 0.45f;
                extent.Z *= 0.65f;
                origin.Z -= extent.Z * 0.0f;

                Structs::FVector corners[8] = {
                    {-extent.X, -extent.Y, -extent.Z},
                    {extent.X, -extent.Y, -extent.Z},
                    {extent.X, extent.Y, -extent.Z},
                    {-extent.X, extent.Y, -extent.Z},
                    {-extent.X, -extent.Y, extent.Z},
                    {extent.X, -extent.Y, extent.Z},
                    {extent.X, extent.Y, extent.Z},
                    {-extent.X, extent.Y, extent.Z}};

                for (int i = 0; i < 8; ++i)
                {
                    Structs::FVector local = corners[i] + origin;
                    Structs::FVector world = transform.TransformPosition(local);
                    Structs::FVector screen = Ue4::world_to_screen(world, minimal_view_info, width, height);
                    shared_response.Objects[shared_response.Count].Box3D[i] = screen;
                }

                strncpy(shared_response.Objects[shared_response.Count].Name, class_name.c_str(), MAX_NAME_LENGTH - 1);
                shared_response.Objects[shared_response.Count].Name[MAX_NAME_LENGTH - 1] = '\0';
                shared_response.Objects[shared_response.Count].Location = location;
                shared_response.Count++;
            }
			
			
            if ((strstr(class_name.c_str(), "VH_") || strstr(class_name.c_str(), "Mirado_") || strstr(class_name.c_str(), "CoupeRB_") || strstr(class_name.c_str(), "Rony_") || strstr(class_name.c_str(), "PickUp_") || strstr(class_name.c_str(), "AquaRail_") || strstr(class_name.c_str(), "BP_Bike_") || strstr(class_name.c_str(), "BP_Bike2_") || strstr(class_name.c_str(), "wing_Vehicle_")))
            {
                if (shared_response.Count >= MAX_OBJECTS)
                    break;
                // Class: STExtraVehicleBase.Pawn.Actor.Object
                // // SkeletalMeshComponent* Mesh;//[Offset: 0xb18, Size: 0x8]
                uintptr_t skeletal_mesh_component = Memory::Read<uintptr_t>(actor + 0xb18, target_pid);
                if (!skeletal_mesh_component)
                    continue;

                // Class: STExtraVehicleBase.Pawn.Actor.Object
                // // SkeletalMesh* TPPMesh;//[Offset: 0x1268, Size: 0x8]
                uintptr_t skeletal_mesh = Memory::Read<uintptr_t>(actor + 0x1268, target_pid);
                if (!skeletal_mesh)
                    continue;

                // Class: BoxSphereBounds
                Structs::FBoxSphereBounds bounds = Memory::Read<Structs::FBoxSphereBounds>(skeletal_mesh + 0xac, target_pid);
                Structs::FVector origin = bounds.Origin;
                Structs::FVector extent = bounds.BoxExtent;
                Structs::FTransform transform = Memory::Read<Structs::FTransform>(skeletal_mesh_component + Offset::component_to_world, target_pid);

                extent.X *= 0.45f;
                extent.Y *= 0.45f;
                extent.Z *= 0.65f;
                origin.Z -= extent.Z * 0.5f;

                Structs::FVector corners[8] = {
                    {-extent.X, -extent.Y, -extent.Z},
                    {extent.X, -extent.Y, -extent.Z},
                    {extent.X, extent.Y, -extent.Z},
                    {-extent.X, extent.Y, -extent.Z},
                    {-extent.X, -extent.Y, extent.Z},
                    {extent.X, -extent.Y, extent.Z},
                    {extent.X, extent.Y, extent.Z},
                    {-extent.X, extent.Y, extent.Z}};

                for (int i = 0; i < 8; ++i)
                {
                    Structs::FVector local = corners[i] + origin;
                    Structs::FVector world = transform.TransformPosition(local);
                    Structs::FVector screen = Ue4::world_to_screen(world, minimal_view_info, width, height);
                    shared_response.Objects[shared_response.Count].Box3D[i] = screen;
                }

                strncpy(shared_response.Objects[shared_response.Count].Name, class_name.c_str(), MAX_NAME_LENGTH - 1);
                shared_response.Objects[shared_response.Count].Name[MAX_NAME_LENGTH - 1] = '\0';
                shared_response.Objects[shared_response.Count].Location = location;
                shared_response.Count++;
            }
			


            if (strstr(class_name.c_str(), "Pickup_C") || strstr(class_name.c_str(), "PickUp") || strstr(class_name.c_str(), "BP_Ammo") || strstr(class_name.c_str(), "BP_QK") || strstr(class_name.c_str(), "Wrapper"))
            {
                if (shared_response.Count >= MAX_OBJECTS)
                    break;

                // Class: .PickUpWrapperActor.UAENetActor.LuaActor.Actor.Object
                // // StaticMesh  NAME_CLASS.PickUpWrapperActor.UAENetActor.LuaActor.Actor.Object
                uintptr_t last_valid_component = 0;

                for (uintptr_t offset : {0x8a0, 0x8a8, 0x8b0, 0x8b8, 0x8c0})
                {
                    uintptr_t candidate = Memory::Read<uintptr_t>(actor + offset, target_pid);
                    if (!candidate)
                        continue;

                    uintptr_t static_mesh = Memory::Read<uintptr_t>(candidate + 0x878, target_pid);
                    if (!static_mesh)
                        continue;

                    Structs::FBoxSphereBounds bounds = Memory::Read<Structs::FBoxSphereBounds>(static_mesh + 0x170, target_pid);

                    if (!isnan(bounds.Origin.X) && !isnan(bounds.BoxExtent.X) &&
                        bounds.BoxExtent.X > 0 && bounds.BoxExtent.X < 5000.0f)
                    {
                        last_valid_component = candidate;
                    }
                }

                if (!last_valid_component)
                    continue;

                uintptr_t static_mesh_component = last_valid_component;

                // Class: Class: StaticMeshComponent.MeshComponent.PrimitiveComponent.SceneComponent.ActorComponent.Object
                // // StaticMesh* StaticMesh;//[Offset: 0x878, Size: 0x8]
                uintptr_t static_mesh = Memory::Read<uintptr_t>(static_mesh_component + 0x878, target_pid);

                // Class: StaticMesh.Object
                // // BoxSphereBounds ExtendedBounds;//[Offset: 0x170, Size: 0x1c]
                Structs::FBoxSphereBounds bounds = Memory::Read<Structs::FBoxSphereBounds>(static_mesh + 0x170, target_pid);
                Structs::FVector origin = bounds.Origin;
                Structs::FVector extent = bounds.BoxExtent;
                Structs::FTransform transform = Memory::Read<Structs::FTransform>(static_mesh_component + Offset::component_to_world, target_pid);

                // extent.X *= 0.6f;
                // extent.Y *= 0.6f;
                // extent.Z *= 0.6f;
                // origin.Z -= extent.Z * 0.5f;
                Structs::FVector corners[8] = {
                    {-extent.X, -extent.Y, -extent.Z},
                    {extent.X, -extent.Y, -extent.Z},
                    {extent.X, extent.Y, -extent.Z},
                    {-extent.X, extent.Y, -extent.Z},
                    {-extent.X, -extent.Y, extent.Z},
                    {extent.X, -extent.Y, extent.Z},
                    {extent.X, extent.Y, extent.Z},
                    {-extent.X, extent.Y, extent.Z}};

                for (int i = 0; i < 8; ++i)
                {
                    Structs::FVector local = corners[i] + origin;
                    Structs::FVector world = transform.TransformPosition(local);
                    Structs::FVector screen = Ue4::world_to_screen(world, minimal_view_info, width, height);
                    shared_response.Objects[shared_response.Count].Box3D[i] = screen;
                }

                strncpy(shared_response.Objects[shared_response.Count].Name, class_name.c_str(), MAX_NAME_LENGTH - 1);
                shared_response.Objects[shared_response.Count].Name[MAX_NAME_LENGTH - 1] = '\0';
                shared_response.Objects[shared_response.Count].Location = location;
                shared_response.Count++;
            }
			
			
			
			
			